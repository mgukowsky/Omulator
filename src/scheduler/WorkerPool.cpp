#include "omulator/scheduler/WorkerPool.hpp"

#include <algorithm>
#include <stdexcept>
#include <thread>

namespace omulator::scheduler {

WorkerPool::WorkerPool(const std::size_t numWorkers) : workerPool_(numWorkers) {

}

WorkerPool::~WorkerPool() {

}

const std::vector<Worker>& WorkerPool::worker_pool() const noexcept {
  return workerPool_;
}

} /* namespace omulator::scheduler */