#include "omulator/scheduler/Worker.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <thread>

using namespace std::chrono_literals;

TEST(Worker_test, addSingleJob) {
  omulator::scheduler::Worker worker;

  EXPECT_TRUE(worker.job_queue().empty())
    << "The Worker's work queue should initially be empty";

  int i = 1;
  std::promise<void> readyPromise;

  worker.add_job([&]{
    ++i;
    readyPromise.set_value();
  });

  readyPromise.get_future().wait();

  EXPECT_EQ(2, i) << "Workers execute submitted tasks in a timely manner";
}