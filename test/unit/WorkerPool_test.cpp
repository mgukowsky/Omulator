#include "omulator/scheduler/WorkerPool.hpp"

#include <gtest/gtest.h>

#include <array>
#include <future>
#include <memory_resource>
#include <thread>

namespace {
auto memRsrc = std::pmr::get_default_resource();
}

TEST(WorkerPool_test, jobDistribution) {
  constexpr int numThreads = 4;

  omulator::scheduler::WorkerPool wp(numThreads, memRsrc);

  EXPECT_EQ(numThreads, wp.worker_pool().size());

  std::array<std::promise<void>, numThreads> startSignals, readySignals, doneSignals;

  for(int i = 0; i < numThreads; ++i) {
    wp.add_job([&, i] {
      startSignals.at(i).set_value();
      readySignals.at(i).get_future().wait();
    });
  };

  for(int i = 0; i < numThreads; ++i) {
    startSignals.at(i).get_future().wait(); 
  }

  {
    const auto stats = wp.stats();

    for(int i = 0; i < numThreads; ++i) {
      EXPECT_EQ(0, stats.at(i).numJobs);
    }
  }

  for(int i = 0; i < numThreads; ++i) {
    wp.add_job([&, i] { doneSignals.at(i).set_value(); });
  }

  // We make this additional no-op job high priority so it executes before the lower priority jobs
  // that set the doneSignals
  wp.add_job([] {}, omulator::scheduler::Priority::HIGH);

  {
    const auto stats = wp.stats();

    // The additional HIGH priority job we just added should be delegated to the first worker
    EXPECT_EQ(2, stats.at(0).numJobs)
      << "WorkerPools should properly distribute workloads across all Workers";

    for(int i = 1; i < numThreads; ++i) {
      EXPECT_EQ(1, stats.at(i).numJobs)
        << "WorkerPools should properly distribute workloads across all Workers";
    }
  }

  for(int i = 0; i < numThreads; ++i) {
    readySignals.at(i).set_value();
  }

  for(int i = 0; i < numThreads; ++i) {
    doneSignals.at(i).get_future().wait();
  }

  {
    const auto stats = wp.stats();
    for(int i = 0; i < numThreads; ++i) {
      EXPECT_EQ(0, stats.at(i).numJobs);
    }
  }
}
