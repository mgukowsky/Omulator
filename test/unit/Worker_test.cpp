#include "omulator/scheduler/Worker.hpp"

#include "mocks/ClockMock.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <memory_resource>
#include <thread>
#include <vector>

namespace {
omulator::scheduler::Worker::WorkerGroup_t workerGroup;

auto memRsrc = std::pmr::get_default_resource();
}  // namespace

using namespace std::chrono_literals;

TEST(Worker_test, addSingleJob) {
  ClockMock                   clock(std::chrono::steady_clock::now());
  omulator::scheduler::Worker worker(workerGroup, clock, memRsrc);

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
  ClockMock                   clock(std::chrono::steady_clock::now());
  omulator::scheduler::Worker worker(workerGroup, clock, memRsrc);

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

TEST(Worker_test, ignoreJobTest) {
  ClockMock                   clock(std::chrono::steady_clock::now());
  omulator::scheduler::Worker worker(workerGroup, clock, memRsrc);

  std::promise<void> startSignal, readySignal, doneSignal;

  // A dummy job to hold the worker in stasis until we're ready.
  worker.add_job([&] {
    startSignal.set_value();
    readySignal.get_future().wait();
  });

  startSignal.get_future().wait();

  int i = 0;

  worker.add_job([&] { ++i; }, omulator::scheduler::Priority::IGNORE);
  worker.add_job([&] { ++i; }, omulator::scheduler::Priority::NORMAL);
  worker.add_job([&] { doneSignal.set_value(); }, omulator::scheduler::Priority::LOW);

  EXPECT_EQ(2, worker.num_jobs())
    << "Jobs with priority set to Priority::IGNORE should never be enqueued via Worker::add_job()";

  readySignal.set_value();
  doneSignal.get_future().wait();

  // Technically there is a race condition possible here since the LOW priority task that unblocks
  // the main thread will be executed before the lowest priority IGNORE task can be executed, but
  // since the IGNORE task will never be executed anyway we don't really care.
  EXPECT_EQ(1, i) << "Jobs with priority set to Priority::IGNORE should never be executed";
}

TEST(Worker_test, nullJobTest) {
  ClockMock                   clock(std::chrono::steady_clock::now());
  omulator::scheduler::Worker worker(workerGroup, clock, memRsrc);

  std::promise<void> startSignal, doneSignal;

  // Without this there would be a race condition between the first call on the main thread to
  // pop_job() below and the Worker's internal thread which calls pop_job on its own thread.
  worker.add_job([&] {
    startSignal.set_value();
    doneSignal.get_future().wait();
  });

  startSignal.get_future().wait();

  // Again, by holding the thread in statis above, we ensure that the job added here will only ever
  // be popped by the pop_job() call below this.
  worker.add_job([] {});
  EXPECT_EQ(omulator::scheduler::Priority::NORMAL, worker.pop_job().priority);
  EXPECT_EQ(omulator::scheduler::Priority::IGNORE, worker.pop_job().priority)
    << "Worker::pop_job should return a null job with Priority::IGNORE when there are no jobs "
       "remaining in the Worker's queue";

  doneSignal.set_value();
}

TEST(Worker_test, jobStealTest) {
  ClockMock                                  clock(std::chrono::steady_clock::now());
  omulator::scheduler::Worker::WorkerGroup_t localWorkerGroup;

  std::promise<void> startSignal1, startSignal2, readySignal1, readySignal2, doneSignal1,
    doneSignal2;

  omulator::scheduler::Worker &worker1 = *(localWorkerGroup.emplace_back(
    std::make_unique<omulator::scheduler::Worker>(localWorkerGroup, clock, memRsrc)));
  omulator::scheduler::Worker &worker2 = *(localWorkerGroup.emplace_back(
    std::make_unique<omulator::scheduler::Worker>(localWorkerGroup, clock, memRsrc)));

  // A dummy job to hold the worker in stasis until we're ready.
  worker1.add_job([&] {
    startSignal1.set_value();
    readySignal1.get_future().wait();
  });

  // ditto
  worker2.add_job([&] {
    startSignal2.set_value();
    readySignal2.get_future().wait();
  });

  startSignal1.get_future().wait();
  startSignal2.get_future().wait();

  int           i     = 0;
  constexpr int MAGIC = 0xABCD;

  // Add a job to worker1's queue...
  worker1.add_job([&] {
    i = MAGIC;
    doneSignal2.set_value();
  });
  EXPECT_EQ(1, worker1.num_jobs());
  EXPECT_EQ(0, worker2.num_jobs());
  EXPECT_EQ(0, i);

  // ...but then leave worker1 blocked and UNblock worker2, which should cause worker2 to steal the
  // job from worker1. Use doneSignal2 to block on the main thread until worker2 steals the job from
  // worker1 and executes it.
  readySignal2.set_value();
  doneSignal2.get_future().wait();

  EXPECT_EQ(0, worker1.num_jobs()) << "A Worker should steal a job from another Worker when "
                                      "periodically wakes up and has no work to do";
  EXPECT_EQ(0, worker2.num_jobs()) << "A Worker should steal a job from another Worker when "
                                      "periodically wakes up and has no work to do";
  EXPECT_EQ(MAGIC, i) << "A Worker should steal a job from another Worker when periodically wakes "
                         "up and has no work to do";

  worker1.add_job([&] { doneSignal1.set_value(); });

  readySignal1.set_value();
  doneSignal1.get_future().wait();
}
