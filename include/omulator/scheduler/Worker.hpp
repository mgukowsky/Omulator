#pragma once

#include "omulator/Latch.hpp"
#include "omulator/scheduler/jobs.hpp"
#include "omulator/util/Pimpl.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <future>
#include <list>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace omulator::scheduler {

/**
 * A worker class representing a single thread of execution. The workers have no knowledge
 * of what the overarching scheduling model looks like; external entities are responsible
 * for enqueueing jobs which respect this scheduling model.
 */
class Worker {
public:
  using WorkerGroup_t = std::vector<std::unique_ptr<Worker>>;

  /**
   * Blocks until the underlying thread has started and is ready to receive work.
   *
   * We pass in a reference to a group of other Workers from which this Worker can steal jobs. This
   * avoids us having to pass in a reference to the entire WorkerPool that contains this worker.
   *
   * We choose to pass in a memory resource so that workers have the option to receive an efficient
   * allocator to use with their internal job queue data structure.
   */
  Worker(WorkerGroup_t &workerGroup, std::pmr::memory_resource *memRsrc);

  /**
   * Blocks until the currently executing task has finished.
   */
  ~Worker();

  Worker(const Worker &) = delete;
  Worker &operator=(const Worker &) = delete;
  Worker(Worker &&)                 = delete;
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
    Callable &&                         work,
    const omulator::scheduler::Priority priority = omulator::scheduler::Priority::NORMAL) {
    static_assert(std::is_invocable_r_v<void, Callable>,
                  "Jobs submitted to a Worker must return void and take no arguments");

    {
      if(priority == Priority::IGNORE) {
        return;
      }

      std::scoped_lock queueLock(jobQueueLock_);

      // Insert the job right before the first job with the next highest priority, or at the end of
      // the queue if there is no such job.
      // TODO: should be const iter?
      auto jobIt = jobQueue_.begin();
      for(; jobIt != jobQueue_.end(); ++jobIt) {
        if(jobIt->priority < priority) {
          break;
        }
      }

      jobQueue_.emplace(jobIt, std::forward<Callable>(work), priority);
    }

    jobCV_.notify_one();
  }

  /**
   * Returns a read-only version of the underlying work queue.
   */
  std::size_t num_jobs() const noexcept;

  /**
   * Pops the highest priority job off of the Worker's jobQueue_ and returns it, or returns
   * a null job (with Priority::IGNORE) if there are no jobs in the queue.
   *
   * LOCKS jobQueueLock_.
   */
  Job_ty pop_job();

  /**
   * Returns the ID of the underlying thread.
   */
  std::thread::id thread_id() const noexcept;

private:
  Latch_ty startupLatch_;

  // TODO **IMPORTANT**: Should maybe be a spinlock?
  using Lock_ty = std::mutex;

  /**
   * A reference to other Workers that this Worker may steal jobs from. N.B. that this group may
   * contain this Worker!
   */
  WorkerGroup_t &workerGroup_;

  /**
   * The work queue of what needs to be done. We use a deque rather that a priority_queue
   * b/c the deque is a great enabler for work stealing by other Workers: this worker can
   * pop high-priority work off the front and other threads can pop low-priority work off
   * the back (or pull the next-highest priority task if needed).
   *
   * We use a list since we need access to standard queue functionalities like push/pop, in addition
   * to random access insertion and deletion for work-stealing functionality.
   */
  std::pmr::list<Job_ty> jobQueue_;

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

  /**
   * Get a reference to the highest priority job in the Worker's jobQueue_.
   */
  Job_ty &peek_job_();

  /**
   * Find the highest priority job out of all the workers in workerGroup_, steal it, and execute it,
   * or do nothing if no such job can be found.
   */
  void steal_job_();

  void thread_proc_();
};

} /* namespace omulator::scheduler */
