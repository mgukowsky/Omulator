#include "omulator/scheduler/Scheduler.hpp"

#include "omulator/di/TypeHash.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <thread>

using namespace std::chrono_literals;

using omulator::di::TypeHash;

namespace {
constexpr auto SCHEDULER_INTERVAL = 5ms;
}  // namespace

namespace omulator::scheduler {

Scheduler::Scheduler(const std::size_t          numWorkers,
                     IClock                    &clock,
                     std::pmr::memory_resource *memRsrc,
                     msg::MailboxRouter        &mailboxRouter)
  : clock_(clock), done_(false), mailbox_(mailboxRouter.claim_mailbox(TypeHash<Scheduler>)) {
  for(std::size_t i = 0; i < numWorkers; ++i) {
    workerPool_.emplace_back(std::make_unique<Worker>(
      Worker::StartupBehavior::SPAWN_THREAD, workerPool_, clock_, memRsrc));
  }
  mailbox_.on(static_cast<U32>(Messages::STOP), [&](const void *) { set_done(); });
}

void Scheduler::scheduler_main() {
  while(!done_) {
    std::this_thread::sleep_for(SCHEDULER_INTERVAL);
    mailbox_.recv();
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

} /* namespace omulator::scheduler */
