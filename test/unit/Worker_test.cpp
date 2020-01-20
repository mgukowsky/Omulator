#include "omulator/scheduler/Worker.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

TEST(Worker_test, addSingleJob) {
  omulator::scheduler::Worker worker;

  EXPECT_TRUE(worker.job_queue().empty()) << "The Worker's work queue should initially be empty";

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
  omulator::scheduler::Worker worker;

  std::promise<void> promise1, promise2;

  // A dummy job to hold the worker in stasis until we're ready.
  worker.add_job([&] { promise1.get_future().wait(); });

  std::vector<int> v;
  const int normalPriority = static_cast<int>(omulator::scheduler::Priority::NORMAL);

  // If priority is obeyed, the vector will be filled with 9..0. If priority is ignored,
  // then the vector will be random or filled with 0..9.
  for(int i = 0; i < 10; ++i) {
    worker.add_job([&, i] { v.push_back(i); },
                   omulator::scheduler::Priority{static_cast<omulator::U8>(normalPriority + i)});
  }

  worker.add_job([&] { promise2.set_value(); }, omulator::scheduler::Priority::LOW);

  promise1.set_value();
  promise2.get_future().wait();

  for(int i = 0; i < 10; ++i) {
    EXPECT_EQ(i, v.back()) << "Workers should select tasks from their job queue based on priority";
    v.pop_back();
  }
}