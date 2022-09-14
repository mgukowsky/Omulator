#include "omulator/msg/MailboxRouter.hpp"

#include "mocks/LoggerMock.hpp"
#include "mocks/PrimitiveIOMock.hpp"
#include "test/Sequencer.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <thread>
#include <vector>

using omulator::U64;
using omulator::msg::MailboxReceiver;
using omulator::msg::MailboxRouter;
using omulator::msg::MailboxSender;
using omulator::msg::Message;
using omulator::msg::MessageQueue;
using omulator::msg::MessageQueueFactory;
using omulator::msg::MessageType;
using omulator::test::Sequencer;

TEST(MailboxRouter_test, usageTest) {
  constexpr U64 LIFE        = 42;
  constexpr U64 LIFEPLUSONE = 43;
  constexpr U64 LIFEPLUSTWO = 44;

  LoggerMock          logger;
  MessageQueueFactory mqf(logger);
  MailboxRouter       mr(logger, mqf);

  MailboxReceiver mrecv  = mr.claim_mailbox<int>();
  MailboxSender   msend1 = mr.get_mailbox<int>();
  MailboxSender   msend2 = mr.get_mailbox<int>();

  MessageQueue *pmq1 = msend1.get_mq();
  EXPECT_NE(nullptr, pmq1) << "MailboxSender::get_mq() should not return a nullptr";

  pmq1->push(MessageType::DEMO_MSG_A, LIFE);
  pmq1->push(MessageType::DEMO_MSG_A, LIFEPLUSONE);
  msend1.send(pmq1);

  MessageQueue *pmq2 = msend2.get_mq();
  EXPECT_NE(nullptr, pmq2) << "MailboxSender::get_mq() should not return a nullptr";

  pmq2->push(MessageType::DEMO_MSG_A, LIFEPLUSTWO);
  msend2.send(pmq2);

  std::vector<U64> vals;

  mrecv.recv([&](const Message &msg) {
    if(msg.type == MessageType::DEMO_MSG_A) {
      vals.push_back(msg.payload);
    }
  });

  EXPECT_EQ(LIFE, vals.at(0)) << "MailboxRouters should allow for endpoints to be properly set up "
                                 "and, subsequently, send and receive messages, and respect the "
                                 "order in which messages are sent";
  EXPECT_EQ(LIFEPLUSONE, vals.at(1))
    << "MailboxRouters should allow for endpoints to be properly set up "
       "and, subsequently, send and receive messages, and respect the order in which messages are "
       "sent";
  EXPECT_EQ(LIFEPLUSTWO, vals.at(2))
    << "MailboxRouters should allow for endpoints to be properly set up "
       "and, subsequently, send and receive messages, and respect the order in which messages are "
       "sent";
}

TEST(MailboxRouter_test, doubleClaimTest) {
  LoggerMock          logger;
  MessageQueueFactory mqf(logger);
  MailboxRouter       mr(logger, mqf);

  [[maybe_unused]] MailboxReceiver mrecv = mr.claim_mailbox<int>();
  EXPECT_THROW(mr.claim_mailbox<int>(), std::runtime_error)
    << "MailboxRouters should only allow a MailboxReceiver corresponding to a given endpoint to be "
       "claimed once";
}

TEST(MailboxRouter_test, multithreaded) {
  Sequencer           sequencer(2);
  LoggerMock          logger;
  MessageQueueFactory mqf(logger);
  MailboxRouter       mr(logger, mqf);

  MailboxReceiver mrecv = mr.claim_mailbox<int>();
  MailboxSender   msend = mr.get_mailbox<int>();

  U64 i = 0;

  std::jthread thrd([&] {
    sequencer.advance_step(1);
    mrecv.recv([&](const Message &msg) {
      if(msg.type == MessageType::DEMO_MSG_A) {
        i = msg.payload;
      }
    });
    sequencer.advance_step(2);
  });

  sequencer.wait_for_step(1);

  MessageQueue *pmq = msend.get_mq();
  pmq->push(MessageType::DEMO_MSG_A, 1);
  msend.send(pmq);

  sequencer.wait_for_step(2);
  EXPECT_EQ(1, i) << "MailboxEndpoint::recv should block until messages are received";
}
