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

namespace {

constexpr U64 LIFE          = 42;
constexpr U64 LIFEPLUSONE   = 43;
constexpr U64 LIFEPLUSTWO   = 44;
constexpr U64 LIFEPLUSTHREE = 45;

}  // namespace

TEST(MailboxRouter_test, usageTest) {
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

  mrecv.on_trivial_payload<U64>(MessageType::DEMO_MSG_A,
                                [&](const U64 payload) { vals.push_back(payload); });
  mrecv.recv();

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

  mrecv.on_trivial_payload<U64>(MessageType::DEMO_MSG_A, [&](const U64 payload) { i = payload; });

  std::jthread thrd([&] {
    sequencer.advance_step(1);
    mrecv.recv();
    sequencer.advance_step(2);
  });

  sequencer.wait_for_step(1);

  MessageQueue *pmq = msend.get_mq();
  pmq->push(MessageType::DEMO_MSG_A, 1);
  msend.send(pmq);

  sequencer.wait_for_step(2);
  EXPECT_EQ(1, i) << "MailboxEndpoint::recv should block until messages are received";
}

TEST(MailboxRouter_test, onMethods) {
  LoggerMock          logger;
  MessageQueueFactory mqf(logger);
  MailboxRouter       mr(logger, mqf);

  MailboxReceiver mrecv = mr.claim_mailbox<int>();
  MailboxSender   msend = mr.get_mailbox<int>();

  std::vector<U64> vals;

  MessageQueue *mq = msend.get_mq();

  mq->push(MessageType::DEMO_MSG_A, LIFE);
  msend.send(mq);

  mrecv.on(MessageType::DEMO_MSG_A, [&] { vals.push_back(LIFE); });
  mrecv.recv();

  EXPECT_EQ(vals.size(), 1);
  EXPECT_EQ(vals.at(0), LIFE);

  mq = msend.get_mq();

  int i = 0x1234;
  mq->push(MessageType::DEMO_MSG_B, i);
  msend.send(mq);

  // Conversion to trivial type
  mrecv.on_trivial_payload<int>(
    MessageType::DEMO_MSG_B, [&](const int payload) { vals.push_back(static_cast<U64>(payload)); });
  mrecv.recv();
  EXPECT_EQ(vals.size(), 2);
  EXPECT_EQ(vals.at(1), 0x1234);

  mq = msend.get_mq();

  int  i2 = LIFEPLUSONE;
  int *pi = &i2;
  mq->push(MessageType::DEMO_MSG_C, pi);
  msend.send(mq);

  // Conversion to pointer
  mrecv.on_trivial_payload<int *>(MessageType::DEMO_MSG_C, [&](const int *payload) {
    vals.push_back(static_cast<U64>(*payload));
  });
  mrecv.recv();
  EXPECT_EQ(vals.size(), 3);
  EXPECT_EQ(vals.at(2), LIFEPLUSONE);

  mq = msend.get_mq();

  mq->push_managed_payload<int>(MessageType::DEMO_MSG_D, static_cast<int>(LIFEPLUSTWO));
  msend.send(mq);

  // Managed payload
  mrecv.on_managed_payload<int>(MessageType::DEMO_MSG_D, [&](const int &payload) {
    vals.push_back(static_cast<U64>(payload));
  });
  mrecv.recv();
  EXPECT_EQ(vals.size(), 4);
  EXPECT_EQ(vals.at(3), LIFEPLUSTWO);

  mq = msend.get_mq();

  int  i3  = LIFEPLUSTHREE;
  int *pi2 = &i3;
  mq->push(MessageType::DEMO_MSG_E, pi2);
  msend.send(mq);

  // Managed payload
  mrecv.on_unmanaged_payload<int>(MessageType::DEMO_MSG_E,
                                  [&](int &payload) { vals.push_back(static_cast<U64>(payload)); });
  mrecv.recv();
  EXPECT_EQ(vals.size(), 5);
  EXPECT_EQ(vals.at(4), LIFEPLUSTHREE);
}