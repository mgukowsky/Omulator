#include "omulator/Subsystem.hpp"

#include "omulator/msg/MailboxRouter.hpp"

#include "mocks/LoggerMock.hpp"
#include "mocks/PrimitiveIOMock.hpp"
#include "test/Sequencer.hpp"

#include <gtest/gtest.h>

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

class TestSubsys : public Subsystem {
public:
  TestSubsys(
    ILogger &logger, MailboxReceiver receiver, MailboxSender sender, U64 &i, Sequencer &sequencer)
    : Subsystem(logger, "TestSubsys", receiver, sender), i_{i}, sequencer_{sequencer} { }
  ~TestSubsys() override = default;

  void message_proc(const Message &msg) override {
    if(msg.type == MessageType::DEMO_MSG_A) {
      i_ = msg.payload;
      sequencer_.advance_step(1);
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

  MailboxReceiver mrecv = mr.claim_mailbox<int>();
  MailboxSender   msend = mr.get_mailbox<int>();

  U64        i = 0;
  TestSubsys subsys(logger, mrecv, msend, i, sequencer);

  auto mq = msend.get_mq();
  mq->push(MessageType::DEMO_MSG_A, 42);
  mq->seal();
  msend.send(mq);

  sequencer.wait_for_step(1);

  EXPECT_EQ(i, 42) << "A specialized subsystem should execute its message_proc() member function "
                      "when messages are sent to the Subsystem";
}
