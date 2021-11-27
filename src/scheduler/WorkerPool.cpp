#include "omulator/scheduler/WorkerPool.hpp"

#include <algorithm>
#include <stdexcept>
#include <thread>

namespace omulator::scheduler {

WorkerPool::WorkerPool(const std::size_t numWorkers, std::pmr::memory_resource *memRsrc) {
  for(std::size_t i = 0; i < numWorkers; ++i) {
    workerPool_.emplace_back(std::make_unique<Worker>(workerPool_, memRsrc));
  }
}

WorkerPool::~WorkerPool() { }

std::size_t WorkerPool::size() const noexcept { return workerPool_.size(); }

const std::vector<WorkerPool::WorkerStats> WorkerPool::stats() const {
  std::scoped_lock lck(poolLock_);

  std::vector<WorkerStats> stats(workerPool_.size());

  for(std::size_t i = 0; i < workerPool_.size(); ++i) {
    stats.at(i).numJobs = workerPool_.at(i)->num_jobs();
  }

  return stats;
}

} /* namespace omulator::scheduler */
