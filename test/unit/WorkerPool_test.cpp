#include "omulator/scheduler/WorkerPool.hpp"

#include <gtest/gtest.h>

#include <memory_resource>
#include <thread>

namespace {
auto memRsrc = std::pmr::get_default_resource();
}

TEST(WorkerPool_test, foo) {
  const int numThreads = 2;

  omulator::scheduler::WorkerPool wp(numThreads, memRsrc);

  EXPECT_EQ(numThreads, wp.worker_pool().size());
}
