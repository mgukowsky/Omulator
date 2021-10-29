#pragma once

#include "omulator/Latch.hpp"
#include "omulator/scheduler/jobs.hpp"
#include "omulator/util/Pimpl.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>

namespace omulator::scheduler {

/**
 * A worker class representing a single thread of execution. The workers have no knowledge
 * of what the overarching scheduling model looks like; external entities are responsible
 * for enqueueing jobs which respect this scheduling model.
 */
class Worker {
public:
  /**
   * Blocks until the underlying thread has started and is ready to receive work.
   */
  Worker();

  /**
   * Blocks until the currently executing task has finished.
   */
  ~Worker();

  Worker(const Worker &) = delete;
  Worker &operator=(const Worker &) = delete;
  Worker(Worker &&) = delete;
  Worker &operator=(Worker &&) = delete;

  /**
   * Add a task to this Worker's work queue. The priority will determine where in the queue the
   * task is placed.
   *
   * @param work
   *   The task to perform. Should be a callable taking no arguments and returning void. N.B. we
   *   take this in as a universal reference as this may be given as a one-off lambda rvalue given
   *   at the call site or a reference to an lvalue functor defined elsewhere.
   *
   * @param priority
   *   The priority of the task; the higher the priority the sooner the task will be scheduled for
   *   execution.
   */
  template<typename Callable>
  void add_job(
    Callable &&work,
    const omulator::scheduler::Priority priority = omulator::scheduler::Priority::NORMAL) {
    static_assert(std::is_invocable_r_v<void, Callable>,
                  "Jobs submitted to a Worker must return void and take no arguments");

    {
      std::scoped_lock queueLock(jobQueueLock_);

      jobQueue_.emplace(jobQueue_.end(), std::forward<Callable>(work), priority);
      std::push_heap(jobQueue_.begin(), jobQueue_.end(), JOB_COMPARATOR);
    }

    jobCV_.notify_one();
  }

  /**
   * Returns a read-only version of the underlying work queue.
   */
  std::size_t num_jobs() const noexcept;

  /**
   * Returns the ID of the underlying thread.
   */
  std::thread::id thread_id() const noexcept;

private:
  Latch_ty startupLatch_;

  // TODO **IMPORTANT**: Should maybe be a spinlock?
  using Lock_ty = std::mutex;

  /**
   * The work queue of what needs to be done. We use a deque rather that a priority_queue
   * b/c the deque is a great enabler for work stealing by other Workers: this worker can
   * pop high-priority work off the front and other threads can pop low-priority work off
   * the back (or pull the next-highest priority task if needed).
   *
   * We also use a deque rather than a vector here (std::vector is also compatible with
   * the set of STL heap functions we'll be using) since the most common operation for
   * the work queue will be pushing onto the back and popping off of the front, and
   * a deque has O(1) performance for these operations.
   */
  std::deque<Job_ty> jobQueue_;

  /**
   * Used to protect access to jobQueue_. Also used by jobCV_ to make help ensure that the
   * thread does not perform a wakeup check while another thread is manipulating the job queue.
   */
  Lock_ty jobQueueLock_;

  /**
   * A condition variable used to signal the underlying thread when it has work to be done.
   */
  std::condition_variable jobCV_;

  /**
   * Indicates when the underlying thread should exit.
   */
  std::atomic_bool done_;

  // N.B. since the thread uses the this pointer, keep this as the last member
  // initialized in the header, such that compilers should wait to set up the thread
  // until everthing else in the object is ready to go, and whine if we do anything
  // to the contrary.
  std::thread thread_;

  void thread_proc_();

  //  class WorkerImpl;
  //  omulator::util::Pimpl<WorkerImpl> impl_;
};

} /* namespace omulator::scheduler */
