#pragma once

#include "omulator/IClock.hpp"
#include "omulator/msg/MailboxRouter.hpp"
#include "omulator/oml_types.hpp"
#include "omulator/scheduler/Worker.hpp"

#include <atomic>
#include <cassert>
#include <limits>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <utility>
#include <vector>

namespace omulator::scheduler {

/**
 * A threadpool class which dispatches jobs to Workers.
 */
class Scheduler {
public:
  // Messages that the scheduler responds to
  enum class Messages : U32 { STOP };

  struct WorkerStats {
    std::size_t numJobs;
  };

  /**
   * Blocks until all threads are started and ready to accept work. Creates a pool of workers
   * based on the argument given.
   *
   * @param numWorkers
   *  The number of Worker objects to create in the pool. Recommended to not exceed the value
   *  reported by std::hardware_concurrency, while accounting for any threads that are currently
   *  running.
   */
  Scheduler(const std::size_t          numWorkers,
            IClock                    &clock,
            std::pmr::memory_resource *memRsrc,
            msg::MailboxRouter        &mailboxRouter);

  /**
   * Blocks until all threads have completed their current task and stopped execution.
   */
  ~Scheduler() = default;

  Scheduler(const Scheduler &) = delete;
  Scheduler &operator=(const Scheduler &) = delete;
  Scheduler(Scheduler &&)                 = delete;
  Scheduler &operator=(Scheduler &&) = delete;

  /**
   * Submit a task to the threadpool. The Worker which will receive the task is selected as follows:
   *    -Iterate through all Workers, and select the first worker found with zero jobs in its queue
   *    -Otherwise, select the worker with the least amount of work in its queue. If there is a tie,
   *      choose the worker according to its order in the Worker pool.
   *
   * LOCKS poolLock_
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
    Callable                          &&work,
    const omulator::scheduler::Priority priority = omulator::scheduler::Priority::NORMAL) {
    static_assert(std::is_invocable_r_v<void, Callable>,
                  "Jobs submitted to a Scheduler must return void and take no arguments");

    std::scoped_lock lck(poolLock_);

    Worker     *bestFitWorker = nullptr;
    std::size_t minNumJobs    = std::numeric_limits<std::size_t>::max();

    for(std::unique_ptr<Worker> &pWorker : workerPool_) {
      Worker     &worker  = *pWorker;
      std::size_t numJobs = worker.num_jobs();
      if(numJobs == 0) {
        bestFitWorker = &worker;
        break;
      }

      if(numJobs < minNumJobs) {
        minNumJobs    = numJobs;
        bestFitWorker = &worker;
      }
    }

    assert(bestFitWorker != nullptr);
    bestFitWorker->add_job(std::forward<Callable>(work), priority);
  }

  /**
   * This is the main loop that the scheduler executes.
   *
   * My original desire was to have the OS periodically interrupt a thread to schedule a single
   * iteration of this function on a Worker with the highest priority. While this type of design is
   * possible with a Unix signal-based approach, the APIs which provide periodic callbacks on other
   * platforms (e.g. Windows, SDL), require that the runtime spin up at least one thread to
   * implement the periodics. Given that my desire is to have this application fully manage its
   * threads, it feels undesireable to have the runtime step on the applications toes by injecting
   * its own threads into the mix. Therefore, the best solution is to call this function from a
   * dedicated thread within the application.
   */
  void scheduler_main();

  /**
   * Set done_ to true, causing scheduler_main() to exit
   */
  void set_done() noexcept;

  std::size_t size() const noexcept;

  /**
   * Gathers and returns information about the Scheduler.
   *
   * LOCKS poolLock_
   */
  const std::vector<WorkerStats> stats() const;

private:
  // TODO **IMPORTANT**: Should maybe be a spinlock?
  using Lock_ty = std::mutex;

  // Marked mutable so that const methods can be atomic and lock this
  mutable Lock_ty poolLock_;

  // We wrap each Worker in a std::unique_ptr to prevent errors arising from the fact that Workers
  // are neither copy- nor move-constructible.
  Worker::WorkerGroup_t workerPool_;

  IClock &clock_;

  std::atomic_bool done_;

  msg::Mailbox &mailbox_;
};

}  // namespace omulator::scheduler
