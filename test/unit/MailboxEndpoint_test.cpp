#include "omulator/msg/MailboxEndpoint.hpp"

#include "mocks/LoggerMock.hpp"

#include <gtest/gtest.h>

using omulator::msg::MailboxEndpoint;
using omulator::msg::Message;
using omulator::msg::MessageQueueFactory;
using omulator::msg::MessageType;

TEST(MailboxEndpoint_test, usageTest) {
  LoggerMock          logger;
  MessageQueueFactory mqf(logger);
  MailboxEndpoint     me(0, mqf);

  EXPECT_FALSE(me.claimed()) << "MailboxEndpoints should be initialized in an unclaimed state";

  me.claim();

  EXPECT_TRUE(me.claimed()) << "MailboxEndpoint::claim should claim an endpoint";

  auto *mq = me.get_mq();
  EXPECT_NE(nullptr, mq) << "MailboxEndpoint::get_mq should never return a nullptr";

  constexpr int LIFE = 42;
  mq->push(MessageType::DEMO_MSG_A, LIFE);

  me.send(mq);

  int i = 0;

  me.recv([&](const Message &msg) {
    if(msg.type == MessageType::DEMO_MSG_A) {
      i = msg.payload;
    }
  });

  EXPECT_EQ(LIFE, i) << "MailboxEndpoints should properly send and recv messages";
}
