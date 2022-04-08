#include "omulator/scheduler/Scheduler.hpp"

#include "omulator/util/to_underlying.hpp"

#include "mocks/ClockMock.hpp"
#include "mocks/LoggerMock.hpp"
#include "mocks/PrimitiveIOMock.hpp"
#include "mocks/exception_handler_mock.hpp"
#include "test/Sequencer.hpp"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <latch>
#include <memory>
#include <memory_resource>
#include <thread>
#include <vector>

using ::testing::Exactly;

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
constexpr std::size_t                NUM_THREADS = 4;
auto                                 memRsrc     = std::pmr::get_default_resource();
std::unique_ptr<LoggerMock>          pLogger;
std::unique_ptr<Injector>            pInjector;
std::unique_ptr<omulator::ClockMock> pClock;
std::unique_ptr<MailboxRouter>       pMailboxRouter;
std::unique_ptr<Scheduler>           pScheduler;
}  // namespace

class Scheduler_test : public ::testing::Test {
protected:
  void SetUp() {
    pLogger.reset(new LoggerMock);
    pInjector.reset(new Injector);

    pInjector->addRecipe<ObjectPool<MessageBuffer>>(
      [](Injector &inj) { return inj.containerize(new ObjectPool<MessageBuffer>(0x10)); });
    pInjector->addRecipe<ObjectPool<Package>>(
      [](Injector &inj) { return inj.containerize(new ObjectPool<Package>(0x10)); });

    pClock.reset(new omulator::ClockMock(std::chrono::steady_clock::now()));
    pMailboxRouter.reset(new MailboxRouter(*pLogger, *pInjector));
    pScheduler.reset(new Scheduler(NUM_THREADS, *pClock, memRsrc, *pMailboxRouter, *pLogger));
  }

  void TearDown() {
    // We _HAVE_ to delete the logger mock here, as mock objects need to be destroyed
    // after tests end and before GoogleMock begins its global teardown, otherwise we
    // will get an exception thrown. See https://github.com/google/googletest/issues/1963
    pLogger.reset();
    pInjector.reset();
    pClock.reset();
    pMailboxRouter.reset();
    pScheduler.reset();
  }
};

TEST_F(Scheduler_test, jobDistribution) {
  Scheduler &wp = *pScheduler;

  EXPECT_EQ(NUM_THREADS, wp.size());

  std::vector<std::unique_ptr<std::latch>> startSignals, readySignals, doneSignals;

  for(std::size_t i = 0; i < NUM_THREADS; ++i) {
    startSignals.emplace_back(std::make_unique<std::latch>(1));
    readySignals.emplace_back(std::make_unique<std::latch>(1));
    doneSignals.emplace_back(std::make_unique<std::latch>(1));
  }

  for(size_t i = 0; i < NUM_THREADS; ++i) {
    wp.add_job_immediate([&, i] {
      startSignals.at(i)->count_down();
      readySignals.at(i)->wait();
    });
  };

  for(size_t i = 0; i < NUM_THREADS; ++i) {
    startSignals.at(i)->wait();
  }

  {
    const auto stats = wp.stats();

    for(size_t i = 0; i < NUM_THREADS; ++i) {
      EXPECT_EQ(0, stats.at(i).numJobs);
    }
  }

  for(size_t i = 0; i < NUM_THREADS; ++i) {
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

    for(size_t i = 1; i < NUM_THREADS; ++i) {
      EXPECT_EQ(1, stats.at(i).numJobs)
        << "Schedulers should properly distribute workloads across all Workers";
    }
  }

  for(size_t i = 0; i < NUM_THREADS; ++i) {
    readySignals.at(i)->count_down();
  }

  for(size_t i = 0; i < NUM_THREADS; ++i) {
    doneSignals.at(i)->wait();
  }

  {
    const auto stats = wp.stats();
    for(size_t i = 0; i < NUM_THREADS; ++i) {
      EXPECT_EQ(0, stats.at(i).numJobs);
    }
  }
}

TEST_F(Scheduler_test, simpleDeferredJob) {
  omulator::ClockMock            &clock     = *pClock;
  omulator::TimePoint_t           now       = clock.now();
  omulator::scheduler::Scheduler &scheduler = *pScheduler;

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
    1s);

  now += 2s;
  clock.set_now(now);
  clock.wake_sleepers();
  sequencer.wait_for_step(2);

  EXPECT_EQ(2, i) << "A deferred job should be invoked by the scheduler once its deadline expires";

  Mailbox &mailbox = pMailboxRouter->get_mailbox<Scheduler>();

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

TEST_F(Scheduler_test, cancelJob) {
  omulator::ClockMock  &clock = *pClock;
  omulator::TimePoint_t now   = clock.now();

  // For this specific test we HAVE to create a scheduler which uses a single thread. We'll be
  // canceling a higher priority job while blocking a sequencer on a lower priority job, and if this
  // test executes properly on a single thread then we can properly evaluate that the scheduler will
  // cancel (and this skip) the highern priority job while still executing and syncing on the lower
  // priority job. We also create a MailboxRouter here, since passing the one created in the SetUp()
  // function would result in the Scheduler mailbox being claimed more than once (by the Scheduler
  // in SetUp() and the one created here).
  MailboxRouter                  mailboxRouter(*pLogger, *pInjector);
  omulator::scheduler::Scheduler scheduler(1, *pClock, memRsrc, mailboxRouter, *pLogger);

  Sequencer sequencer(3);

  using namespace std::chrono_literals;

  std::jthread t([&] {
    sequencer.advance_step(1);
    scheduler.scheduler_main();
    sequencer.advance_step(3);
  });

  sequencer.wait_for_step(1);
  int i = 1;

  const auto jobHandle = scheduler.add_job_deferred(
    [&] {
      ++i;
      sequencer.advance_step(2);
    },
    1s);

  scheduler.add_job_deferred([&] { sequencer.advance_step(2); },
                             1s,
                             omulator::scheduler::Scheduler::SchedType::ONE_OFF,
                             omulator::scheduler::Priority::LOW);

  scheduler.cancel_job(jobHandle);

  now += 2s;
  clock.set_now(now);
  clock.wake_sleepers();
  sequencer.wait_for_step(2);

  EXPECT_EQ(1, i)
    << "A cancelled job should not be invoked by the scheduler once its deadline expires";

  // Ensure that we warn when a given job is canceled more than once...
  EXPECT_CALL(*pLogger, warn).Times(Exactly(1));
  scheduler.cancel_job(jobHandle);

  // ...and also when a nonexistent handle is given
  EXPECT_CALL(*pLogger, warn).Times(Exactly(1));
  scheduler.cancel_job(0xFFFF'FFFF);

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

  // Ensure we trigger the EXPECT_CALL assertions by deleting the underlying object before the test
  // function exits
  pLogger.reset();
}

TEST_F(Scheduler_test, multipleDeferredJobs) {
  omulator::ClockMock  &clock = *pClock;
  omulator::TimePoint_t now   = clock.now();

  Sequencer sequencer(5);

  Scheduler &scheduler = *pScheduler;

  using namespace std::chrono_literals;

  std::jthread t([&] {
    sequencer.advance_step(1);
    scheduler.scheduler_main();
    sequencer.advance_step(5);
  });

  sequencer.wait_for_step(1);
  std::array<int, 3> nums{0, 0, 0};

  scheduler.add_job_deferred(
    [&] {
      nums.at(0) = 1;
      sequencer.advance_step(2);
    },
    // N.B. this will test if the deadline will expire if the dealine is greater
    // than or EQUAL to the current time!
    2s);

  scheduler.add_job_deferred(
    [&] {
      nums.at(1) = 2;
      sequencer.advance_step(3);
    },
    3s);

  scheduler.add_job_deferred(
    [&] {
      nums.at(2) = 3;
      sequencer.advance_step(4);
    },
    5s);

  now += 2s;
  clock.set_now(now);
  clock.wake_sleepers();
  sequencer.wait_for_step(2);

  EXPECT_EQ(1, nums.at(0))
    << "A deferred job should be invoked by the scheduler once its deadline expires";
  EXPECT_EQ(0, nums.at(1))
    << "A deferred job should not be invoked by the scheduler until its deadline expires";
  EXPECT_EQ(0, nums.at(2))
    << "A deferred job should not be invoked by the scheduler until its deadline expires";

  now += 2s;
  clock.set_now(now);
  clock.wake_sleepers();
  sequencer.wait_for_step(3);

  EXPECT_EQ(1, nums.at(0))
    << "A deferred job should be invoked by the scheduler once its deadline expires";
  EXPECT_EQ(2, nums.at(1))
    << "A deferred job should be invoked by the scheduler once its deadline expires";
  EXPECT_EQ(0, nums.at(2))
    << "A deferred job should not be invoked by the scheduler until its deadline expires";

  now += 2s;
  clock.set_now(now);
  clock.wake_sleepers();
  sequencer.wait_for_step(4);

  EXPECT_EQ(1, nums.at(0))
    << "A deferred job should be invoked by the scheduler once its deadline expires";
  EXPECT_EQ(2, nums.at(1))
    << "A deferred job should be invoked by the scheduler once its deadline expires";
  EXPECT_EQ(3, nums.at(2))
    << "A deferred job should be invoked by the scheduler once its deadline expires";

  Mailbox &mailbox = pMailboxRouter->get_mailbox<Scheduler>();

  Package *pkg = mailbox.open_pkg();
  pkg->alloc_msg(to_underlying(Scheduler::Messages::STOP));
  mailbox.send(pkg);

  now += 2s;

  // Wake once to have the scheduler call Mailbox::recv() to send the stop message...
  clock.wake_sleepers();

  // ...and wake it up once more since the check for !done_ doesn't happen until after
  // scheduler_main goes to sleep again after calling Mailbox::recv()
  clock.wake_sleepers();

  sequencer.wait_for_step(5);

  // Ensure we trigger the EXPECT_CALL assertions by deleting the underlying object before the test
  // function exits
  pLogger.reset();
}

TEST_F(Scheduler_test, periodicJob) {
  omulator::ClockMock  &clock = *pClock;
  omulator::TimePoint_t now   = clock.now();

  // For this specific test we HAVE to create a scheduler which uses a single thread. We'll be
  // canceling a higher priority job while blocking a sequencer on a lower priority job, and if this
  // test executes properly on a single thread then we can properly evaluate that the scheduler will
  // cancel (and this skip) the highern priority job while still executing and syncing on the lower
  // priority job. We also create a MailboxRouter here, since passing the one created in the SetUp()
  // function would result in the Scheduler mailbox being claimed more than once (by the Scheduler
  // in SetUp() and the one created here).
  MailboxRouter                  mailboxRouter(*pLogger, *pInjector);
  omulator::scheduler::Scheduler scheduler(1, *pClock, memRsrc, mailboxRouter, *pLogger);

  Sequencer sequencer(6);

  using namespace std::chrono_literals;

  std::jthread t([&] {
    sequencer.advance_step(1);
    scheduler.scheduler_main();
    sequencer.advance_step(6);
  });

  sequencer.wait_for_step(1);

  int i = 1;

  // An invalid delay should not be scheduled and should error out
  EXPECT_CALL(*pLogger, error).Times(Exactly(1));
  const auto invalidHandle =
    scheduler.add_job_deferred([&] { ++i; },
                               0s,
                               omulator::scheduler::Scheduler::SchedType::PERIODIC,
                               omulator::scheduler::Priority::HIGH);

  EXPECT_EQ(omulator::scheduler::Scheduler::INVALID_JOB_HANDLE, invalidHandle)
    << "A Scheduler should report an attempt to schedule a periodic job withan invalid delay";

  const auto jobHandle = scheduler.add_job_deferred(
    [&] {
      ++i;
      sequencer.advance_step(sequencer.current_step() + 1);
    },
    2s,
    omulator::scheduler::Scheduler::SchedType::PERIODIC);

  now += 2s;
  clock.set_now(now);
  clock.wake_sleepers();
  sequencer.wait_for_step(2);

  EXPECT_EQ(2, i) << "A periodic deferred job should be invoked by the scheduler once its initial "
                     "deadline expires";

  now += 2s;
  clock.set_now(now);
  clock.wake_sleepers();
  sequencer.wait_for_step(3);

  EXPECT_EQ(3, i) << "A periodic deferred job should be periodically invoked by the scheduler each "
                     "time its deadline expires";

  now += 2s;
  clock.set_now(now);
  clock.wake_sleepers();
  sequencer.wait_for_step(4);

  EXPECT_EQ(4, i) << "A periodic deferred job should be periodically invoked by the scheduler each "
                     "time its deadline expires";

  scheduler.cancel_job(jobHandle);
  scheduler.add_job_deferred([&] { sequencer.advance_step(5); },
                             3s,
                             omulator::scheduler::Scheduler::SchedType::ONE_OFF,
                             omulator::scheduler::Priority::LOW);

  // Jump way in the future to simulate a few iterations that the cancelled job might have executed
  now += 10s;
  clock.set_now(now);
  clock.wake_sleepers();
  sequencer.wait_for_step(5);

  EXPECT_EQ(4, i) << "A periodic deferred job should no longer be executed once it it cancelled";

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

  sequencer.wait_for_step(6);

  // Ensure we trigger the EXPECT_CALL assertions by deleting the underlying object before the test
  // function exits
  pLogger.reset();
}

TEST_F(Scheduler_test, periodicNonExclusive) {
  omulator::ClockMock  &clock = *pClock;
  omulator::TimePoint_t now   = clock.now();

  Sequencer sequencer(7);

  Scheduler &scheduler = *pScheduler;

  using namespace std::chrono_literals;

  std::jthread t([&] {
    sequencer.advance_step(1);
    scheduler.scheduler_main();
    sequencer.advance_step(7);
  });

  sequencer.wait_for_step(1);

  std::atomic_bool shouldBlock = true;
  std::atomic_uint i           = 1;

  // PERIODIC_NONEXCLUSIVE jobs should continue to recur as their periodic deadlines expire, even if
  // there are iterations of the job still running when the deadline expires. We simulate this by
  // blocking the initial iteration of the job while having subsequent iterations not block on
  // anything.
  scheduler.add_job_deferred(
    [&] {
      if(shouldBlock) {
        sequencer.advance_step(2);
        i = sequencer.current_step();

        shouldBlock = false;
        sequencer.wait_for_step(5);
        i = 5;
        sequencer.advance_step(6);
      }
      else {
        sequencer.advance_step(sequencer.current_step() + 1);
        i = sequencer.current_step();
      }
    },
    2s,
    omulator::scheduler::Scheduler::SchedType::PERIODIC_NONEXCLUSIVE);

  now += 3s;
  clock.set_now(now);
  clock.wake_sleepers();
  sequencer.wait_for_step(2);

  EXPECT_EQ(2, i)
    << "A non-exclusive periodic job should be executed once when it it initially scheduled";

  now += 2s;
  clock.set_now(now);
  clock.wake_sleepers();
  sequencer.wait_for_step(3);
  EXPECT_EQ(3, i) << "An iteration of a non-exclusive periodic job should always execute when its "
                     "timeout expires even if another iteration of the job is currently running";

  now += 2s;
  clock.set_now(now);
  clock.wake_sleepers();
  sequencer.wait_for_step(4);
  EXPECT_EQ(4, i) << "An iteration of a non-exclusive periodic job should always execute when its "
                     "timeout expires even if another iteration of the job is currently running";

  sequencer.advance_step(5);
  sequencer.wait_for_step(6);
  EXPECT_EQ(5, i) << "A long-running iteration of a non-exclusive periodic job should continue to "
                     "execute even if its periodic deadline recurs while it is still running";

  Mailbox &mailbox = pMailboxRouter->get_mailbox<Scheduler>();

  Package *pkg = mailbox.open_pkg();
  pkg->alloc_msg(to_underlying(Scheduler::Messages::STOP));
  mailbox.send(pkg);

  now += 2s;

  // Wake once to have the scheduler call Mailbox::recv() to send the stop message...
  clock.wake_sleepers();

  // ...and wake it up once more since the check for !done_ doesn't happen until after
  // scheduler_main goes to sleep again after calling Mailbox::recv()
  clock.wake_sleepers();

  sequencer.wait_for_step(7);

  // Ensure we trigger the EXPECT_CALL assertions by deleting the underlying object before the test
  // function exits
  pLogger.reset();
}
