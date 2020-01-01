#include "omulator/scheduler/Worker.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

TEST(Worker_test, addSingleJob) {
  omulator::scheduler::Worker worker;

  EXPECT_TRUE(worker.job_queue().empty())
    << "The Worker's work queue should initially be empty";

  int i = 1;
  worker.add_job([&]{ ++i; });
  std::this_thread::sleep_for(10ms);

  EXPECT_EQ(2, i) << "Workers execute submitted tasks in a timely manner";
}