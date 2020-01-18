#include "omulator/scheduler/WorkerPool.hpp"

#include <gtest/gtest.h>

#include <thread>

TEST(WorkerPool_test, foo) {
  const int numThreads = 2;
  omulator::scheduler::WorkerPool wp(numThreads);
  EXPECT_EQ(numThreads, wp.worker_pool().size());
}