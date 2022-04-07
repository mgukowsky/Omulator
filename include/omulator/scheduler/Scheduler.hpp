#pragma once

#include "omulator/IClock.hpp"
#include "omulator/ILogger.hpp"
#include "omulator/msg/MailboxRouter.hpp"
#include "omulator/oml_types.hpp"
#include "omulator/scheduler/Worker.hpp"
#include "omulator/util/Pimpl.hpp"
#include "omulator/util/to_underlying.hpp"

#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <queue>
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

  enum class SchedType : U8 { ONE_OFF, PERIODIC };

  struct WorkerStats {
    std::size_t numJobs;
  };

  using JobHandle_t = U64;

  static constexpr U32                       SCHEDULER_HZ = 120;
  static constexpr std::chrono::milliseconds SCHEDULER_PERIOD_MS =
    std::chrono::milliseconds(1000 / SCHEDULER_HZ);
  static constexpr JobHandle_t INVALID_JOB_HANDLE = 0xFFFF'FFFF'FFFF'FFFF;

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
            msg::MailboxRouter        &mailboxRouter,
            ILogger                   &logger);

  /**
   * Blocks until all threads have completed their current task and stopped execution.
   */
  ~Scheduler();

  Scheduler(const Scheduler &) = delete;
  Scheduler &operator=(const Scheduler &) = delete;
  Scheduler(Scheduler &&)                 = delete;
  Scheduler &operator=(Scheduler &&) = delete;

  /**
   * Execute a job at a given point in the future.
   *
   * Returns a handle to the job which can be passed to cancel_job, or INVALID_JOB_HANDLE if the job
   * couldn't be scheduled.
   */
  JobHandle_t add_job_deferred(std::function<void()> work,
                               const Duration_t      delay,
                               const SchedType       schedType = SchedType::ONE_OFF,
                               const Priority        priority  = Priority::NORMAL);

  /**
   * Submit a task to the threadpool for immediate execution. The Worker which will receive the task
   * is selected as follows:
   *  -Iterate through all Workers, and select the first worker found with zero jobs in its queue
   *  -Otherwise, select the worker with the least amount of work in its
   * queue. If there is a tie, choose the worker according to its order in the Worker pool.
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
  void add_job_immediate(std::function<void()> work, const Priority priority = Priority::NORMAL);

  /**
   * Stop a scheduled job from executing.
   */
  void cancel_job(const JobHandle_t id);

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
  struct Scheduler_impl;
  util::Pimpl<Scheduler_impl> impl_;

  // TODO **IMPORTANT**: Should maybe be a spinlock?
  using Lock_ty = std::mutex;

  struct JobQueueEntry_t {
    Job_ty      job;
    TimePoint_t deadline;
    Duration_t  delay;
    JobHandle_t id;
    SchedType   schedType;
  };

  /**
   * The underlying business logic to handle deferred jobs. N.B. that this doesn't lock anything,
   * and expects calling methods to handle locks around the jobQueue state.
   */
  bool add_job_deferred_with_id_(std::function<void()> work,
                                 const TimePoint_t     deadline,
                                 const Duration_t      delay,
                                 const SchedType       schedType,
                                 const Priority        priority,
                                 const JobHandle_t     id);

  JobHandle_t iota_() noexcept;

  void schedule_periodic_iteration_(const JobQueueEntry_t &entry);

  /**
   * This lock is used for the schedulers state, mainly the workerPool_, but NOT the job queues
   * managed by impl_, which have their own locks.
   */
  mutable Lock_ty poolLock_;

  std::map<JobHandle_t, U32> periodicIterationTracker_;
  mutable Lock_ty            periodicIterationTrackerLock_;

  // We wrap each Worker in a std::unique_ptr to prevent errors arising from the fact that Workers
  // are neither copy- nor move-constructible.
  Worker::WorkerGroup_t workerPool_;

  IClock &clock_;

  std::atomic_bool         done_;
  std::atomic<JobHandle_t> iotaVal_;

  msg::Mailbox &mailbox_;
  ILogger      &logger_;
};

}  // namespace omulator::scheduler
