#pragma once

#include "omulator/scheduler/Worker.hpp"

#include <cassert>
#include <limits>
#include <mutex>
#include <utility>
#include <vector>

namespace omulator::scheduler {

/**
 * A threadpool class which dispatches jobs to Workers.
 */
class WorkerPool {
public:
  /**
   * Blocks until all threads are started and ready to accept work. Creates a pool of workers
   * based on the argument given.
   *
   * @param numWorkers
   *  The number of Worker objects to create in the pool. Recommended to not exceed the value
   *  reported by std::hardware_concurrency, while accounting for any threads that are currently
   *  running.
   */
  WorkerPool(const std::size_t numWorkers);

  /**
   * Blocks until all threads have completed their current task and stopped execution.
   */
  ~WorkerPool();

  WorkerPool(const WorkerPool &) = delete;
  WorkerPool &operator=(const WorkerPool &) = delete;
  WorkerPool(WorkerPool &&)                 = delete;
  WorkerPool &operator=(WorkerPool &&) = delete;

  /**
   * Get a read-only reference to the underlying threadpool.
   */
  const std::vector<Worker> &worker_pool() const noexcept;

  /**
   * Submit a task to the threadpool. The Worker which will receive the task is selected as follows:
   *    -Iterate through all Workers, and select the first worker found with zero jobs in its queue
   *    -Otherwise, select the worker with the least amount of work in its queue. If there is a tie,
   *      choose the worker according to its order in the queue.
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
                  "Jobs submitted to a WorkerPool must return void and take no arguments");

    std::scoped_lock lck(poolLock_);

    Worker *    bestFitWorker = nullptr;
    std::size_t minNumJobs    = std::numeric_limits<std::size_t>::max();

    for(Worker &worker : workerPool_) {
      std::size_t numJobs = worker.num_jobs();
      if(numJobs == 0) {
        bestFitWorker = &worker;
        break;
      }
      else {
        if(numJobs < minNumJobs) {
          minNumJobs    = numJobs;
          bestFitWorker = &worker;
        }
      }
    }

    assert(bestFitWorker != nullptr);
    bestFitWorker->add_job(std::forward<Callable>(work), priority);
  }

private:
  // TODO **IMPORTANT**: Should maybe be a spinlock?
  using Lock_ty = std::mutex;

  Lock_ty poolLock_;

  std::vector<Worker> workerPool_;

  /**
   * Sorts workers first by the number of jobs in their queue, and then by their thread ID. Sorting
   * by thread ID is a small optimization which allows us to repeatedly re-use the same thread,
   * potentially making things easier on the scheduler.
   *
   * N.B. we don't lock the workers while doing this, so it's possible that the size of the
   * job_queue can change while this comparison is taking place, but that's an acceptable risk for
   * us, for now at least.
   */
  static constexpr auto WORKER_COMPARATOR = [](const Worker &a, const Worker &b) {
    const auto aSiz = a.num_jobs();
    const auto bSiz = b.num_jobs();

    if(aSiz == bSiz) {
      return a.thread_id() < b.thread_id();
    }
    else {
      return aSiz > bSiz;
    }
  };
};

}  // namespace omulator::scheduler
