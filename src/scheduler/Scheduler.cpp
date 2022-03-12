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

namespace {
constexpr auto SCHEDULER_INTERVAL = 5ms;
}  // namespace

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
    [](const JobQueueEntry_t &a, const JobQueueEntry_t &b) { return a.deadline < b.deadline; };

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

U64 Scheduler::add_job_deferred(std::function<void()>               work,
                                const omulator::TimePoint_t         deadline,
                                const omulator::scheduler::Priority priority) {
  auto &jobQueuePair = impl_->jobQueues_.at(to_underlying(priority));

  std::scoped_lock lck(jobQueuePair.second);

  Scheduler_impl::JobQueue_t &jobQueue = jobQueuePair.first;

  const U64 id = iota_();

  jobQueue.emplace_back(JobQueueEntry_t{
    Job_ty{work, priority},
    deadline, id
  });
  std::push_heap(jobQueue.begin(), jobQueue.end(), Scheduler_impl::jobQueueComparator);

  return id;
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

void Scheduler::cancel_job(const U64 id) {
  bool found = false;

  for(auto &jobQueuePair : impl_->jobQueues_) {
    std::scoped_lock lck(jobQueuePair.second);

    Scheduler_impl::JobQueue_t &jobQueue = jobQueuePair.first;

    auto matchIt = std::find_if(jobQueue.begin(),
                                jobQueue.end(),
                                [&](const JobQueueEntry_t &entry) { return entry.id == id; });

    if(matchIt != jobQueue.end()) {
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
  while(!done_) {
    const auto currentTime  = clock_.now();
    const auto nextDeadline = currentTime + SCHEDULER_INTERVAL;

    mailbox_.recv();

    // Run through the queues in reverse order since higher priorities will have higher indices in
    // the jobQueues_ array
    for(auto jobQueueIt = impl_->jobQueues_.rbegin(); jobQueueIt != impl_->jobQueues_.rend();
        ++jobQueueIt)
    {
      // Lock this specific queue while we iterate through it
      std::scoped_lock lck(jobQueueIt->second);

      Scheduler_impl::JobQueue_t &jobQueue = jobQueueIt->first;

      while(!jobQueue.empty()) {
        const JobQueueEntry_t &jobQueueEntry = jobQueue.front();

        if(jobQueueEntry.deadline > currentTime) {
          break;
        }
        else {
          add_job_immediate(jobQueueEntry.job.task, jobQueueEntry.job.priority);
          std::pop_heap(jobQueue.begin(), jobQueue.end(), Scheduler_impl::jobQueueComparator);
          jobQueue.pop_back();
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

U64 Scheduler::iota_() noexcept {
  const U64 val = iotaVal_.fetch_add(1, std::memory_order_acq_rel);
  return val;
}

} /* namespace omulator::scheduler */
