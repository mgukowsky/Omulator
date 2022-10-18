#include "omulator/Subsystem.hpp"

#include "omulator/msg/MailboxRouter.hpp"

#include "mocks/LoggerMock.hpp"
#include "mocks/PrimitiveIOMock.hpp"
#include "mocks/exception_handler_mock.hpp"
#include "test/Sequencer.hpp"

#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Exactly;
using ::testing::HasSubstr;

using omulator::ILogger;
using omulator::Subsystem;
using omulator::U64;
using omulator::msg::MailboxReceiver;
using omulator::msg::MailboxRouter;
using omulator::msg::MailboxSender;
using omulator::msg::Message;
using omulator::msg::MessageQueueFactory;
using omulator::msg::MessageType;
using omulator::test::Sequencer;
using omulator::util::TypeHash;

class TestSubsys : public Subsystem {
public:
  TestSubsys(ILogger &logger, MailboxRouter &mbrouter, U64 &i, Sequencer &sequencer)
    : Subsystem(logger, "TestSubsys", mbrouter, TypeHash<TestSubsys>),
      i_{i},
      sequencer_{sequencer} {
    start_();
  }
  ~TestSubsys() override = default;

  void message_proc(const Message &msg) override {
    if(msg.type == MessageType::DEMO_MSG_A) {
      i_ = msg.payload;
      sequencer_.advance_step(1);
    }
    else {
      Subsystem::message_proc(msg);
    }
  }

  U64       &i_;
  Sequencer &sequencer_;
};

TEST(Subsystem_test, simpleSubsystem) {
  Sequencer  sequencer(1);
  LoggerMock logger;

  MessageQueueFactory mqf(logger);
  MailboxRouter       mr(logger, mqf);

  MailboxSender msend = mr.get_mailbox<TestSubsys>();

  U64        i = 0;
  TestSubsys subsys(logger, mr, i, sequencer);

  auto mq = msend.get_mq();
  mq->push(MessageType::DEMO_MSG_A, 42);
  mq->push(MessageType::DEMO_MSG_B, 43);
  mq->seal();

  // Ensure that calling Subsystem::message_proc on messages not processed by derived Subsystems
  // produces the appropriate warning.
  EXPECT_CALL(logger, warn(HasSubstr("Unprocessed message: "), _)).Times(Exactly(1));
  msend.send(mq);

  sequencer.wait_for_step(1);

  EXPECT_EQ(i, 42) << "A specialized subsystem should execute its message_proc() member function "
                      "when messages are sent to the Subsystem";
}
