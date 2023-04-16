#include "omulator/msg/MailboxEndpoint.hpp"

#include "mocks/LoggerMock.hpp"

#include <gtest/gtest.h>

#include <vector>

using omulator::U64;
using omulator::msg::MailboxEndpoint;
using omulator::msg::Message;
using omulator::msg::MessageQueueFactory;
using omulator::msg::MessageType;

using ::testing::_;
using ::testing::Exactly;
using ::testing::HasSubstr;

namespace {
constexpr int LIFE = 42;
}  // namespace

TEST(MailboxEndpoint_test, usageTest) {
  LoggerMock          logger;
  MessageQueueFactory mqf(logger);
  MailboxEndpoint     me(0, logger, mqf);

  EXPECT_FALSE(me.claimed()) << "MailboxEndpoints should be initialized in an unclaimed state";

  me.claim();

  EXPECT_TRUE(me.claimed()) << "MailboxEndpoint::claim should claim an endpoint";

  auto *mq = me.get_mq();
  EXPECT_NE(nullptr, mq) << "MailboxEndpoint::get_mq should never return a nullptr";

  mq->push(MessageType::DEMO_MSG_A, LIFE);

  EXPECT_FALSE(mq->sealed()) << "MailboxEndpoint::send() should call MessageQueue::seal()";
  me.send(mq);
  EXPECT_TRUE(mq->sealed()) << "MailboxEndpoint::send() should call MessageQueue::seal()";

  U64 i = 0;

  me.on(MessageType::DEMO_MSG_A, [&](const Message &msg) { i = msg.payload; });

  me.recv();
  EXPECT_EQ(LIFE, i) << "MailboxEndpoints should properly send and recv messages";
}

// TODO: test off, on w/ other trivial types, on_managed_payload, log missed msgs, esp. log if
// missed managed msg

TEST(MailboxEndpoint_test, onOff) {
  LoggerMock          logger;
  MessageQueueFactory mqf(logger);
  MailboxEndpoint     me(0, logger, mqf);

  me.claim();

  auto *mq = me.get_mq();
  mq->push(MessageType::DEMO_MSG_A, LIFE);
  mq->push(MessageType::DEMO_MSG_B, LIFE);
  me.send(mq);

  std::vector<U64> vals;

  me.on(MessageType::DEMO_MSG_A, [&](const Message &msg) { vals.push_back(msg.payload); });
  me.on(MessageType::DEMO_MSG_B, [&](const Message &msg) { vals.push_back(msg.payload); });

  me.recv();

  EXPECT_EQ(vals.size(), 2) << "MailboxEndpoint::on should invoke the appropriate callback";

  me.off(MessageType::DEMO_MSG_A);

  mq = me.get_mq();
  mq->push(MessageType::DEMO_MSG_A, LIFE);
  mq->push(MessageType::DEMO_MSG_B, LIFE);
  me.send(mq);

  EXPECT_CALL(logger, warn(HasSubstr("Dropping message with type "), _)).Times(Exactly(1));
  me.recv();

  EXPECT_EQ(vals.size(), 3) << "MailboxEndpoint::off should remove any callbacks previously "
                               "associated with a given message via on()";

  EXPECT_CALL(logger, warn(HasSubstr("Attempted to invoke MailboxEndpoint::on "), _))
    .Times(Exactly(1));
  me.on(MessageType::DEMO_MSG_B, [&]([[maybe_unused]] const Message &msg) { vals.pop_back(); });

  mq = me.get_mq();
  mq->push(MessageType::DEMO_MSG_B, LIFE);
  me.send(mq);

  me.recv();

  EXPECT_EQ(vals.size(), 4) << "MailboxEndpoint::on should have no effect if there is already a "
                               "callback registered for a given MessageType";

  me.on(MessageType::DEMO_MSG_A, [&]([[maybe_unused]] const Message &msg) { vals.pop_back(); });

  mq = me.get_mq();
  mq->push(MessageType::DEMO_MSG_A, LIFE);
  me.send(mq);

  me.recv();

  EXPECT_EQ(vals.size(), 3)
    << "MailboxEndpoint::on should register a callback for a given MessageType "
       "after a call to MailboxEndpoint::off";
}