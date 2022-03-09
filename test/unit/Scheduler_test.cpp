#include "omulator/scheduler/Scheduler.hpp"

#include "omulator/util/to_underlying.hpp"

#include "mocks/ClockMock.hpp"
#include "mocks/LoggerMock.hpp"
#include "mocks/PrimitiveIOMock.hpp"
#include "mocks/exception_handler_mock.hpp"
#include "test/Sequencer.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <latch>
#include <memory>
#include <memory_resource>
#include <thread>
#include <vector>

using omulator::di::Injector;
using omulator::di::TypeHash;
using omulator::msg::Mailbox;
using omulator::msg::MailboxRouter;
using omulator::msg::MessageBuffer;
using omulator::msg::Package;
using omulator::scheduler::Scheduler;
using omulator::test::Sequencer;
using omulator::util::ObjectPool;
using omulator::util::to_underlying;

namespace {
auto memRsrc = std::pmr::get_default_resource();
}  // namespace

TEST(Scheduler_test, jobDistribution) {
  constexpr size_t numThreads = 4;

  LoggerMock logger;
  Injector   injector;
  injector.addRecipe<ObjectPool<MessageBuffer>>(
    [](Injector &inj) { return inj.containerize(new ObjectPool<MessageBuffer>(0x10)); });
  injector.addRecipe<ObjectPool<Package>>(
    [](Injector &inj) { return inj.containerize(new ObjectPool<Package>(0x10)); });
  MailboxRouter mailboxRouter(logger, injector);

  omulator::ClockMock                      clock(std::chrono::steady_clock::now());
  omulator::scheduler::Scheduler wp(numThreads, clock, memRsrc, mailboxRouter);

  EXPECT_EQ(numThreads, wp.size());

  std::vector<std::unique_ptr<std::latch>> startSignals, readySignals, doneSignals;

  for(std::size_t i = 0; i < numThreads; ++i) {
    startSignals.emplace_back(std::make_unique<std::latch>(1));
    readySignals.emplace_back(std::make_unique<std::latch>(1));
    doneSignals.emplace_back(std::make_unique<std::latch>(1));
  }

  for(size_t i = 0; i < numThreads; ++i) {
    wp.add_job_immediate([&, i] {
      startSignals.at(i)->count_down();
      readySignals.at(i)->wait();
    });
  };

  for(size_t i = 0; i < numThreads; ++i) {
    startSignals.at(i)->wait();
  }

  {
    const auto stats = wp.stats();

    for(size_t i = 0; i < numThreads; ++i) {
      EXPECT_EQ(0, stats.at(i).numJobs);
    }
  }

  for(size_t i = 0; i < numThreads; ++i) {
    wp.add_job_immediate([&, i] { doneSignals.at(i)->count_down(); });
  }

  // We make this additional no-op job high priority so it executes before the lower priority jobs
  // that set the doneSignals
  wp.add_job_immediate([] {}, omulator::scheduler::Priority::HIGH);

  {
    const auto stats = wp.stats();

    // The additional HIGH priority job we just added should be delegated to the first worker
    EXPECT_EQ(2, stats.at(0).numJobs)
      << "Schedulers should properly distribute workloads across all Workers";

    for(size_t i = 1; i < numThreads; ++i) {
      EXPECT_EQ(1, stats.at(i).numJobs)
        << "Schedulers should properly distribute workloads across all Workers";
    }
  }

  for(size_t i = 0; i < numThreads; ++i) {
    readySignals.at(i)->count_down();
  }

  for(size_t i = 0; i < numThreads; ++i) {
    doneSignals.at(i)->wait();
  }

  {
    const auto stats = wp.stats();
    for(size_t i = 0; i < numThreads; ++i) {
      EXPECT_EQ(0, stats.at(i).numJobs);
    }
  }
}

TEST(Scheduler_test, simpleDeferredJob) {
  constexpr size_t numThreads = 4;

  LoggerMock logger;
  Injector   injector;
  injector.addRecipe<ObjectPool<MessageBuffer>>(
    [](Injector &inj) { return inj.containerize(new ObjectPool<MessageBuffer>(0x10)); });
  injector.addRecipe<ObjectPool<Package>>(
    [](Injector &inj) { return inj.containerize(new ObjectPool<Package>(0x10)); });
  MailboxRouter mailboxRouter(logger, injector);

  omulator::TimePoint_t          now = std::chrono::steady_clock::now();
  omulator::ClockMock                      clock(now);
  omulator::scheduler::Scheduler scheduler(numThreads, clock, memRsrc, mailboxRouter);

  Sequencer sequencer(3);

  using namespace std::chrono_literals;

  std::jthread t([&] {
    sequencer.advance_step(1);
    scheduler.scheduler_main();
    sequencer.advance_step(3);
  });

  sequencer.wait_for_step(1);
  int i = 1;
  scheduler.add_job_deferred(
    [&] {
      ++i;
      sequencer.advance_step(2);
    },
    now + 1s);

  now += 2s;
  clock.set_now(now);
  clock.wake_sleepers();
  sequencer.wait_for_step(2);

  EXPECT_EQ(2, i) << "A deferred job should be invoked by the scheduler once its deadline expires";

  Mailbox &mailbox = mailboxRouter.get_mailbox<Scheduler>();

  Package *pkg = mailbox.open_pkg();
  pkg->alloc_msg(to_underlying(Scheduler::Messages::STOP));
  mailbox.send(pkg);

  now += 2s;

  // Wake once to have the scheduler call Mailbox::recv() to send the stop message...
  clock.wake_sleepers();

  // ...and wake it up once more since the check for !done_ doesn't happen until after
  // scheduler_main goes to sleep again after calling Mailbox::recv()
  clock.wake_sleepers();

  sequencer.wait_for_step(3);
}
