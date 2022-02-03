#include "omulator/scheduler/Scheduler.hpp"

#include <algorithm>
#include <stdexcept>
#include <thread>

namespace omulator::scheduler {

Scheduler::Scheduler(const std::size_t          numWorkers,
                     IClock                    &clock,
                     std::pmr::memory_resource *memRsrc)
  : clock_(clock) {
  for(std::size_t i = 0; i < numWorkers; ++i) {
    workerPool_.emplace_back(std::make_unique<Worker>(
      Worker::StartupBehavior::SPAWN_THREAD, workerPool_, clock_, memRsrc));
  }
}

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
