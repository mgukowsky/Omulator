#include "omulator/scheduler/Worker.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <memory_resource>
#include <thread>
#include <vector>

namespace {
auto memRsrc = std::pmr::get_default_resource();
}

using namespace std::chrono_literals;

TEST(Worker_test, addSingleJob) {
  omulator::scheduler::Worker worker(memRsrc);

  EXPECT_TRUE(worker.num_jobs() == 0) << "The Worker's work queue should initially be empty";

  int i = 1;

  std::promise<void> readyPromise;

  worker.add_job([&] {
    ++i;
    readyPromise.set_value();
  });

  readyPromise.get_future().wait();

  EXPECT_EQ(2, i) << "Workers execute submitted tasks in a timely manner";
}

TEST(Worker_test, jobPriorityTest) {
  omulator::scheduler::Worker worker(memRsrc);

  std::promise<void> startSignal, readySignal, doneSignal;

  // A dummy job to hold the worker in stasis until we're ready.
  worker.add_job([&] {
    startSignal.set_value();
    readySignal.get_future().wait();
  });

  startSignal.get_future().wait();

  std::vector<int> v;
  const int        normalPriority = static_cast<int>(omulator::scheduler::Priority::NORMAL);

  // If priority is obeyed, the vector will be filled with 9..0. If priority is ignored,
  // then the vector will be random or filled with 0..9.
  for(int i = 0; i < 10; ++i) {
    worker.add_job([&, i] { v.push_back(i); },
                   omulator::scheduler::Priority{static_cast<omulator::U8>(normalPriority + i)});
  }

  worker.add_job([&] { doneSignal.set_value(); }, omulator::scheduler::Priority::LOW);

  readySignal.set_value();
  doneSignal.get_future().wait();

  for(int i = 0; i < 10; ++i) {
    EXPECT_EQ(i, v.back()) << "Workers should select tasks from their job queue based on priority";
    v.pop_back();
  }
}
