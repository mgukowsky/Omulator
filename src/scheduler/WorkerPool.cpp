#include "omulator/scheduler/WorkerPool.hpp"

#include <algorithm>
#include <stdexcept>
#include <thread>

namespace omulator::scheduler {

WorkerPool::WorkerPool(const std::size_t numWorkers, std::pmr::memory_resource *memRsrc) {
  for(std::size_t i = 0; i < numWorkers; ++i) {
    workerPool_.emplace_back(std::make_unique<Worker>(memRsrc));
  }
}

WorkerPool::~WorkerPool() { }

const std::vector<std::unique_ptr<Worker>> &WorkerPool::worker_pool() const noexcept {
  return workerPool_;
}

} /* namespace omulator::scheduler */
