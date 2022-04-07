#include "omulator/scheduler/Scheduler.hpp"

#include "omulator/di/TypeHash.hpp"
#include "omulator/oml_types.hpp"
#include "omulator/util/to_underlying.hpp"

#include <algorithm>
#include <chrono>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <utility>

using namespace std::chrono_literals;

using omulator::di::TypeHash;
using omulator::util::to_underlying;

namespace omulator::scheduler {

/**
 * We have to use a Pimpl for the jobQueues_ member because it uses a lambda type for a comparator.
 * If we were to declare these members directly in the header, we would get compiler errors stemming
 * from the use of the lambda's anonymous type in each translation unit which includes this header
 * (see
 * https://www.reddit.com/r/cpp_questions/comments/8im3h4/warning_class_has_a_field_whose_type_uses_the/).
 * Admittedly not the greatest solution, but arguably cleaner than alternatives like defining a
 * functor or a custom comparator...
 */
struct Scheduler::Scheduler_impl {
  static constexpr auto jobQueueComparator =
    [](const JobQueueEntry_t &a, const JobQueueEntry_t &b) { return a.deadline > b.deadline; };

  using JobQueue_t = std::vector<JobQueueEntry_t>;

  std::array<std::pair<JobQueue_t, std::mutex>, util::to_underlying(Priority::MAX) + 1> jobQueues_;
};

Scheduler::Scheduler(const std::size_t          numWorkers,
                     IClock                    &clock,
                     std::pmr::memory_resource *memRsrc,
                     msg::MailboxRouter        &mailboxRouter,
                     ILogger                   &logger)
  : clock_(clock),
    done_(false),
    iotaVal_(0),
    mailbox_(mailboxRouter.claim_mailbox<Scheduler>()),
    logger_(logger) {
  for(std::size_t i = 0; i < numWorkers; ++i) {
    workerPool_.emplace_back(std::make_unique<Worker>(
      Worker::StartupBehavior::SPAWN_THREAD, workerPool_, clock_, memRsrc));
  }
  mailbox_.on(to_underlying(Messages::STOP), [&](const void *) { set_done(); });
}

Scheduler::~Scheduler() = default;

Scheduler::JobHandle_t Scheduler::add_job_deferred(std::function<void()>               work,
                                                   const Duration_t                    delay,
                                                   const SchedType                     schedType,
                                                   const omulator::scheduler::Priority priority) {
  const JobHandle_t     id = iota_();
  std::function<void()> task;

  if(schedType == SchedType::PERIODIC) {
    std::scoped_lock lck(periodicIterationTrackerLock_);
    periodicIterationTracker_.emplace(id, 0);

    // Create a closure to handle to wrap the work in additional logic which tracks how many copies
    // of the work are currently executing
    auto workClosure = [&, work, id] {
      {
        std::scoped_lock innerLck(periodicIterationTrackerLock_);

        auto innerIterationTrackerEntry = periodicIterationTracker_.find(id);
        if(innerIterationTrackerEntry == periodicIterationTracker_.end()) {
          return;
        }
      }
      work();
      {
        std::scoped_lock innerLck(periodicIterationTrackerLock_);

        auto innerIterationTrackerEntry = periodicIterationTracker_.find(id);
        if(innerIterationTrackerEntry != periodicIterationTracker_.end()) {
          auto &innerIterationTracker = innerIterationTrackerEntry->second;
          --innerIterationTracker;
        }
      }
    };
    task = workClosure;
  }
  else {
    // If the deferred job is a one-off, then we don't need any of the bookkeeping provided by the
    // aforementioned closure.
    task = work;
  }

  std::scoped_lock lck(impl_->jobQueues_.at(to_underlying(priority)).second);
  if(add_job_deferred_with_id_(task, clock_.now() + delay, delay, schedType, priority, id)) {
    return id;
  }
  else {
    return INVALID_JOB_HANDLE;
  }
}

void Scheduler::add_job_immediate(std::function<void()>               work,
                                  const omulator::scheduler::Priority priority) {
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
  bestFitWorker->add_job(std::move(work), priority);
}

void Scheduler::cancel_job(const Scheduler::JobHandle_t id) {
  bool found = false;

  for(auto &jobQueuePair : impl_->jobQueues_) {
    std::scoped_lock lck(jobQueuePair.second);

    Scheduler_impl::JobQueue_t &jobQueue = jobQueuePair.first;

    auto matchIt = std::find_if(jobQueue.begin(),
                                jobQueue.end(),
                                [&](const JobQueueEntry_t &entry) { return entry.id == id; });

    if(matchIt != jobQueue.end()) {
      {
        std::scoped_lock innerLck(periodicIterationTrackerLock_);
        periodicIterationTracker_.erase(matchIt->id);
      }
      jobQueue.erase(matchIt);
      std::make_heap(jobQueue.begin(), jobQueue.end(), Scheduler_impl::jobQueueComparator);

      found = true;
      break;
    }
  }

  if(!found) {
    std::stringstream ss;
    ss << "Attempted to cancel job with id " << id
       << "; this indicates that the job was either cancelled previously or the id of the job is "
          "invalid (i.e. not returned by a call to Scheduler::add_job_*())";
    logger_.warn(ss.str().c_str());
  }
}

void Scheduler::scheduler_main() {
  auto nextDeadline = clock_.now();
  while(!done_) {
    // Make the next deadline relative to the initial timestamp rather than clock_.now() to avoid
    // clock drift. N.B. that this means that if an iteration of the scheduler runs past the next
    // deadline, then the scheduler will continue to execute iterations immediately until it
    // "catches up" with the wall clock.
    nextDeadline += SCHEDULER_PERIOD_MS;

    mailbox_.recv();

    const auto currentTime = clock_.now();

    // Run through the queues in reverse order since higher priorities will have higher indices in
    // the jobQueues_ array
    for(auto jobQueueIt = impl_->jobQueues_.rbegin(); jobQueueIt != impl_->jobQueues_.rend();
        ++jobQueueIt)
    {
      // Lock this specific queue while we iterate through it
      std::scoped_lock lck(jobQueueIt->second);

      Scheduler_impl::JobQueue_t &jobQueue = jobQueueIt->first;

      while(!jobQueue.empty()) {
        std::pop_heap(jobQueue.begin(), jobQueue.end(), Scheduler_impl::jobQueueComparator);
        const JobQueueEntry_t &jobQueueEntry = jobQueue.back();

        if(jobQueueEntry.deadline > currentTime) {
          // Somewhat unintuitive, but pop_heap removes the last element from the heap, even
          // though it is still part of the container. We need to re-add it to the heap to ensure
          // that the container remains a valid heap.
          std::push_heap(jobQueue.begin(), jobQueue.end(), Scheduler_impl::jobQueueComparator);
          break;
        }
        else {
          // We have to remove the entry from the job queue before move forward, since certain
          // control paths here, such as that for PERIODIC, will mutate the end of the container.
          JobQueueEntry_t oldEntry = jobQueueEntry;
          jobQueue.pop_back();

          switch(jobQueueEntry.schedType) {
            case SchedType::ONE_OFF:
              add_job_immediate(oldEntry.job.task, oldEntry.job.priority);
              break;
            case SchedType::PERIODIC:
              schedule_periodic_iteration_(oldEntry);
              break;
            default:
              logger_.error("Could not schedule job with invalid schedule type");
              break;
          }
        }
      }
    }

    clock_.sleep_until(nextDeadline);
  }
}

void Scheduler::set_done() noexcept { done_ = true; }

std::size_t Scheduler::size() const noexcept { return workerPool_.size(); }

const std::vector<Scheduler::WorkerStats> Scheduler::stats() const {
  std::scoped_lock lck(poolLock_);

  std::vector<WorkerStats> stats(workerPool_.size());

  for(std::size_t i = 0; i < workerPool_.size(); ++i) {
    stats.at(i).numJobs = workerPool_.at(i)->num_jobs();
  }

  return stats;
}

bool Scheduler::add_job_deferred_with_id_(std::function<void()>               work,
                                          const TimePoint_t                   deadline,
                                          const Duration_t                    delay,
                                          const SchedType                     schedType,
                                          const omulator::scheduler::Priority priority,
                                          const Scheduler::JobHandle_t        id) {
  // We cannot allow this situation because it could quickly overload the scheduler
  if(schedType == SchedType::PERIODIC && delay < SCHEDULER_PERIOD_MS) {
    std::stringstream ss;
    ss << "Could not schedule periodic job with id " << id << " because its delay of "
       << delay.count() << " was less than the Scheduler's period of "
       << SCHEDULER_PERIOD_MS.count() << "ms";
    logger_.error(ss.str().c_str());

    return false;
  }

  auto &jobQueuePair = impl_->jobQueues_.at(to_underlying(priority));

  Scheduler_impl::JobQueue_t &jobQueue = jobQueuePair.first;

  jobQueue.emplace_back(JobQueueEntry_t{
    Job_ty{work, priority},
    deadline, delay, id, schedType
  });
  std::push_heap(jobQueue.begin(), jobQueue.end(), Scheduler_impl::jobQueueComparator);

  return true;
}

Scheduler::JobHandle_t Scheduler::iota_() noexcept {
  const JobHandle_t val = iotaVal_.fetch_add(1, std::memory_order_acq_rel);
  return val;
}

void Scheduler::schedule_periodic_iteration_(const Scheduler::JobQueueEntry_t &entry) {
  auto      &task = entry.job.task;
  const auto id   = entry.id;

  std::scoped_lock lck(periodicIterationTrackerLock_);
  auto             iterationTrackerEntry = periodicIterationTracker_.find(id);

  // Has the job been cancelled?
  if(iterationTrackerEntry == periodicIterationTracker_.end()) {
    return;
  }

  auto &iterationTracker = iterationTrackerEntry->second;

  ++iterationTracker;
  if(iterationTracker == 1) {
    add_job_immediate(task, entry.job.priority);
  }

  // We call this internal overload directly so that the periodic job can reuse the same
  // ID as the original call to add_job_deferred, which allows cancel_job to still
  // correctly cancel this job on future iterations.
  if(!add_job_deferred_with_id_(task,
                                // We make the new deadline relative to the old deadline
                                // to prevent the gradual drift that would accumulate
                                // over iterations of the periodic if we used
                                // clock_.now() + entry.delay
                                // This also helps prevent larger drift that can be caused
                                // by the scheduler itself missing a deadline
                                entry.deadline + entry.delay,
                                entry.delay,
                                SchedType::PERIODIC,
                                entry.job.priority,
                                id))
  {
    std::stringstream ss;
    ss << "Failed to schedule periodic job iteration for job with id " << id;
    throw std::runtime_error(ss.str());
  }
}

} /* namespace omulator::scheduler */
