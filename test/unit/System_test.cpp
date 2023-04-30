#include "omulator/System.hpp"

#include "mocks/LoggerMock.hpp"
#include "mocks/PrimitiveIOMock.hpp"
#include "mocks/exception_handler_mock.hpp"
#include "test/Sequencer.hpp"

#include <gtest/gtest.h>

#include <functional>
#include <string_view>
#include <vector>

using ::testing::_;
using ::testing::Exactly;
using ::testing::HasSubstr;

using omulator::Component;
using omulator::ComponentList_t;
using omulator::Cycle_t;
using omulator::ILogger;
using omulator::Subsystem;
using omulator::SubsystemList_t;
using omulator::System;
using omulator::U64;
using omulator::di::Injector;
using omulator::msg::MailboxRouter;
using omulator::msg::Message;
using omulator::msg::MessageCallback_t;
using omulator::msg::MessageQueueFactory;
using omulator::msg::MessageType;
using omulator::test::Sequencer;
using omulator::util::TypeHash;
using omulator::util::TypeString;

namespace {

// Pythonic way to take no action for a particular message in a message_proc
[[maybe_unused]] auto pass = []([[maybe_unused]] const Message &msg) {};

class A : public Component {
public:
  A(ILogger &logger, std::vector<std::string_view> &vs, Cycle_t &cycleTracker)
    : Component(logger, TypeString<A>), vs_(vs), cycleTracker_(cycleTracker) { }

  ~A() override = default;

  Cycle_t step(const Cycle_t numCycles) override {
    cycleTracker_ += numCycles;
    vs_.push_back("A");
    return numCycles;
  }

private:
  std::vector<std::string_view> &vs_;
  Cycle_t                       &cycleTracker_;
};

class B : public Component {
public:
  B(ILogger &logger, std::vector<std::string_view> &vs, Cycle_t &cycleTracker)
    : Component(logger, TypeString<B>), vs_(vs), cycleTracker_(cycleTracker) { }

  ~B() override = default;

  Cycle_t step(const Cycle_t numCycles) override {
    cycleTracker_ += numCycles;
    vs_.push_back("B");
    return numCycles;
  }

private:
  std::vector<std::string_view> &vs_;
  Cycle_t                       &cycleTracker_;
};

class SubsysA : public Subsystem {
public:
  SubsysA(ILogger                       &logger,
          MailboxRouter                 &mbrouter,
          std::function<void(const U64)> onDemoMsgA,
          std::function<void()>          onDestruction)
    : Subsystem(logger, TypeString<SubsysA>, mbrouter, TypeHash<SubsysA>),
      onDemoMsgA_(onDemoMsgA),
      onDestruction_(onDestruction) {
    receiver_.on_trivial_payload<U64>(MessageType::DEMO_MSG_A,
                                      [&](const U64 payload) { onDemoMsgA_(payload); });
    start();
  }

  ~SubsysA() override { onDestruction_(); }

private:
  std::function<void(const U64)> onDemoMsgA_;
  std::function<void()>          onDestruction_;
};

}  // namespace

TEST(System_test, emptySystemWarning) {
  LoggerMock logger;
  Injector   injector;

  [[maybe_unused]] System system(logger, "emptySystem", injector);

  EXPECT_CALL(logger, warn(HasSubstr("has no components!"), _)).Times(Exactly(1));
  EXPECT_CALL(logger, warn(HasSubstr("has no subsystems!"), _)).Times(Exactly(1));
  system.step(1);
}

TEST(System_test, simpleSystem) {
  LoggerMock                    logger;
  Injector                      injector;
  std::vector<std::string_view> recorder;

  Cycle_t aCycleTracker = 0;
  Cycle_t bCycleTracker = 0;

  Sequencer     sequencer(2);
  constexpr U64 MAGIC              = 1234;
  U64           i                  = 0;
  U64           destructionTracker = 0;

  injector.bindImpl<ILogger, LoggerMock>();

  injector.addRecipe<omulator::msg::MessageQueueFactory>([](Injector &injectorInstance) {
    static std::atomic<U64> factoryInstanceCounter = 0;
    return new omulator::msg::MessageQueueFactory(
      injectorInstance.get<ILogger>(),
      factoryInstanceCounter.fetch_add(1, std::memory_order_acq_rel));
  });

  injector.addCtorRecipe<MailboxRouter, ILogger &, MessageQueueFactory &>();
  injector.addRecipe<A>([&]([[maybe_unused]] omulator::di::Injector &inj) {
    return new A(inj.get<ILogger>(), recorder, aCycleTracker);
  });
  injector.addRecipe<B>([&]([[maybe_unused]] omulator::di::Injector &inj) {
    return new B(inj.get<ILogger>(), recorder, bCycleTracker);
  });

  {
    [[maybe_unused]] System system(logger, "testsystem", injector);

    // Create the recipe for SubsysA on the child Injector, not the parent. Helps ensure that only
    // the child Injector is managing its own Subsystems.
    system.get_injector().addRecipe<SubsysA>([&]([[maybe_unused]] omulator::di::Injector &inj) {
      return new SubsysA(
        inj.get<ILogger>(),
        inj.get<MailboxRouter>(),
        [&](const U64 payload) {
          i = payload;
          sequencer.advance_step(1);
        },
        [&]() {
          destructionTracker = MAGIC + 1;
          sequencer.advance_step(2);
        });
    });

    system.make_component_list<A, B>();

    system.make_subsystem_list<SubsysA>();

    EXPECT_EQ(3, system.step(3)) << "System::step should return the number of cycles taken";

    EXPECT_EQ(3, aCycleTracker) << "System::step should call the step() function for each of its "
                                   "Components, in the order they "
                                   "are provided to the System instance";
    EXPECT_EQ(3, bCycleTracker) << "System::step should call the step() function for each of its "
                                   "Components, in the order they "
                                   "are provided to the System instance";

    EXPECT_EQ("A", recorder.at(0)) << "System::step should call the step() function for each of "
                                      "its Components, in the order they "
                                      "are provided to the System instance";
    EXPECT_EQ("B", recorder.at(1)) << "System::step should call the step() function for each of "
                                      "its Components, in the order they "
                                      "are provided to the System instance";
    EXPECT_EQ("A", recorder.at(2)) << "System::step should call the step() function for each of "
                                      "its Components, in the order they "
                                      "are provided to the System instance";
    EXPECT_EQ("B", recorder.at(3)) << "System::step should call the step() function for each of "
                                      "its Components, in the order they "
                                      "are provided to the System instance";
    EXPECT_EQ("A", recorder.at(4)) << "System::step should call the step() function for each of "
                                      "its Components, in the order they "
                                      "are provided to the System instance";
    EXPECT_EQ("B", recorder.at(5)) << "System::step should call the step() function for each of "
                                      "its Components, in the order they "
                                      "are provided to the System instance";
    EXPECT_EQ(6, recorder.size()) << "System::step should call the step() function for each of its "
                                     "Components, in the order they "
                                     "are provided to the System instance";

    // N.B. that we need to retrieve the MailboxRouter from the system's Injector, not the parent
    // Injector. If we called get<MailboxRouter>() on the parent Injector, then this would return a
    // new MailboxRouter instance rather than the one used by the child Injector in the system. This
    // is because this is the first call to get<MailboxRouter>() for the parent Injector, whereas
    // get<MailboxRouter>() has previously been called on the child instance. Since there was no
    // instance in the parent Injector, the child Injector created its own self-managed instance of
    // the MailboxRouter dependency. If we had called get<MailboxRouter>() (or called get/creat on a
    // type which had MailboxRouter in its dependency chain) on the parent Injector BEFORE creating
    // the System and its child Injector, then we could call get<MailboxRouter> on EITHER the child
    // or parent Injector.
    system.get_injector().get<MailboxRouter>().get_mailbox<SubsysA>().send_single_message(
      MessageType::DEMO_MSG_A, MAGIC);
    sequencer.wait_for_step(1);

    EXPECT_EQ(MAGIC, i) << "Systems should create subsystems which run in their own thread and "
                           "respond to messages appropriately";
    EXPECT_EQ(0, destructionTracker) << "A System should wait to destroy all of its components and "
                                        "subsystems until its destruction";
  }

  sequencer.wait_for_step(2);
  EXPECT_EQ(MAGIC + 1, destructionTracker)
    << "A System should destroy all of its components and subsystems on destruction";
}
