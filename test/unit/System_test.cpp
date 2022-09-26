#include "omulator/System.hpp"

#include "mocks/LoggerMock.hpp"
#include "mocks/PrimitiveIOMock.hpp"

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
using omulator::make_component_list;
using omulator::make_subsystem_list;
using omulator::Subsystem;
using omulator::SubsystemList_t;
using omulator::System;
using omulator::di::Injector;
using omulator::di::TypeHash;
using omulator::di::TypeString;
using omulator::msg::MailboxRouter;
using omulator::msg::Message;
using omulator::msg::MessageQueueFactory;
using omulator::msg::MessageType;

namespace {

class A : public Component {
public:
  A(ILogger &logger, std::vector<std::string_view> &vs)
    : Component(logger, TypeString<A>), numCycles_(0), vs_(vs) { }
  ~A() override = default;

  Cycle_t step(const Cycle_t numCycles) override {
    numCycles_ += numCycles;
    vs_.push_back("A");
    return numCycles;
  }

  Cycle_t numCycles() const { return numCycles_; }

private:
  Cycle_t                        numCycles_;
  std::vector<std::string_view> &vs_;
};

class B : public Component {
public:
  B(ILogger &logger, std::vector<std::string_view> &vs)
    : Component(logger, TypeString<B>), numCycles_(0), vs_(vs) { }
  ~B() override = default;

  Cycle_t step(const Cycle_t numCycles) override {
    numCycles_ += numCycles;
    vs_.push_back("B");
    return numCycles;
  }

  Cycle_t numCycles() const { return numCycles_; }

private:
  Cycle_t                        numCycles_;
  std::vector<std::string_view> &vs_;
};

class SubsysA : public Subsystem {
public:
  SubsysA(ILogger &logger, MailboxRouter &mbrouter)
    : Subsystem(logger, "SubsysA", mbrouter, TypeHash<SubsysA>) { }
  ~SubsysA() override = default;

  void message_proc(const Message &msg) override {
    if(msg.type == MessageType::DEMO_MSG_A) {
    }
    else {
      Subsystem::message_proc(msg);
    }
  }
};

}  // namespace

TEST(System_test, emptySystemWarning) {
  LoggerMock      logger;
  ComponentList_t components;
  SubsystemList_t subsystems;

  EXPECT_CALL(logger, warn(HasSubstr("created with no components!"), _)).Times(Exactly(1));
  EXPECT_CALL(logger, warn(HasSubstr("created with no subsystems!"), _)).Times(Exactly(1));
  [[maybe_unused]] System system(logger, "emptySystem", components, subsystems);
}

TEST(System_test, simpleSystem) {
  LoggerMock                    logger;
  Injector                      injector;
  std::vector<std::string_view> recorder;

  injector.bindImpl<ILogger, LoggerMock>();
  injector.addCtorRecipe<MessageQueueFactory, ILogger &>();
  injector.addCtorRecipe<MailboxRouter, ILogger &, MessageQueueFactory &>();
  injector.addCtorRecipe<SubsysA, ILogger &, MailboxRouter &>();
  injector.addRecipe<A>([&]([[maybe_unused]] omulator::di::Injector &inj) {
    return new A(inj.get<ILogger>(), recorder);
  });
  injector.addRecipe<B>([&]([[maybe_unused]] omulator::di::Injector &inj) {
    return new B(inj.get<ILogger>(), recorder);
  });

  [[maybe_unused]] System system(logger,
                                 "testsystem",
                                 make_component_list<A, B>(injector),
                                 make_subsystem_list<SubsysA>(injector));

  EXPECT_EQ(3, system.step(3)) << "System::step should return the number of cycles taken";

  EXPECT_EQ(3, injector.get<A>().numCycles())
    << "System::step should call the step() function for each of its Components, in the order they "
       "are provided to the System instance";
  EXPECT_EQ(3, injector.get<B>().numCycles())
    << "System::step should call the step() function for each of its Components, in the order they "
       "are provided to the System instance";

  EXPECT_EQ("A", recorder.at(0))
    << "System::step should call the step() function for each of its Components, in the order they "
       "are provided to the System instance";
  EXPECT_EQ("B", recorder.at(1))
    << "System::step should call the step() function for each of its Components, in the order they "
       "are provided to the System instance";
  EXPECT_EQ("A", recorder.at(2))
    << "System::step should call the step() function for each of its Components, in the order they "
       "are provided to the System instance";
  EXPECT_EQ("B", recorder.at(3))
    << "System::step should call the step() function for each of its Components, in the order they "
       "are provided to the System instance";
  EXPECT_EQ("A", recorder.at(4))
    << "System::step should call the step() function for each of its Components, in the order they "
       "are provided to the System instance";
  EXPECT_EQ("B", recorder.at(5))
    << "System::step should call the step() function for each of its Components, in the order they "
       "are provided to the System instance";
  EXPECT_EQ(6, recorder.size())
    << "System::step should call the step() function for each of its Components, in the order they "
       "are provided to the System instance";
}
