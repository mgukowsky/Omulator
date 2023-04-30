#include "omulator/msg/MessageQueueFactory.hpp"

#include "mocks/LoggerMock.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

using omulator::U64;
using omulator::msg::Message;
using omulator::msg::MessageQueue;
using omulator::msg::MessageQueueFactory;
using omulator::msg::MessageType;

using ::testing::_;
using ::testing::Exactly;
using ::testing::HasSubstr;

TEST(MessageQueueFactory_test, getAndSubmit) {
  LoggerMock          logger;
  MessageQueueFactory mqf(logger, 0);

  MessageQueue mq = mqf.get();

  EXPECT_TRUE(mq.valid()) << "MessageQueueFactory::submit() should call MessageQueue::release()";

  mqf.submit(mq);
  EXPECT_FALSE(mq.valid()) << "MessageQueueFactory::submit() should call MessageQueue::release()";

  // Shouldn't warn about memory leaks since we have called submit() for each MessageQueue acquired
  // through get()
  EXPECT_CALL(logger, error).Times(Exactly(0));
}

TEST(MessageQueueFactory_test, memLeakWarn) {
  LoggerMock          logger;
  MessageQueueFactory mqf(logger, 0);

  MessageQueue                  mq1 = mqf.get();
  [[maybe_unused]] MessageQueue mq2 = mqf.get();

  mqf.submit(mq1);
  EXPECT_CALL(logger, error).Times(Exactly(1));
}

TEST(MessageQueueFactory_test, doubleSubmitWarn) {
  LoggerMock          logger;
  MessageQueueFactory mqf(logger, 0);

  MessageQueue mq = mqf.get();

  mqf.submit(mq);

  EXPECT_CALL(logger, error(HasSubstr("attempted to submit a MessageQueue that is not valid"), _))
    .Times(Exactly(1));
  mqf.submit(mq);
}

TEST(MessageQueueFactory_test, foreignPoolWarn) {
  LoggerMock          logger;
  MessageQueueFactory mqf0(logger, 0);
  MessageQueueFactory mqf1(logger, 1);

  MessageQueue mqf0Instance0 = mqf0.get();
  MessageQueue mqf0Instance1 = mqf0.get();

  mqf0.submit(mqf0Instance0);

  EXPECT_CALL(logger, error(HasSubstr("that did not create it"), _)).Times(Exactly(1));
  mqf1.submit(mqf0Instance1);

  // Even though submit() will not return the foreign memory to the pool, it will still call
  // MessageQueue::release() which will mark the MessageQueue as invalid
  EXPECT_CALL(logger, error(HasSubstr("attempted to submit a MessageQueue that is not valid"), _))
    .Times(Exactly(1));
  mqf0.submit(mqf0Instance1);

  // Because the factory was never able to accept the invalid MessageQueue, MessageQueueFactory's
  // destructor will correctly complain
  EXPECT_CALL(
    logger,
    error(HasSubstr("expected to destroy 2 MessageQueue::Storage_t, but instead destroyed 1"), _))
    .Times(Exactly(1));
}

TEST(MessageQueueFactory_test, memoryReuseTest) {
  LoggerMock          logger;
  MessageQueueFactory mqf(logger, 0);

  MessageQueue mq = mqf.get();

  mqf.submit(mq);

  MessageQueue reusedMQ = mqf.get();
  MessageQueue freshMQ  = mqf.get();

  // Don't actually do this; we are only explicitly calling release on a submitted instance in order
  // to evaluate the address of its storage.
  EXPECT_EQ(mq.release(), reusedMQ.release());
  EXPECT_NE(mq.release(), freshMQ.release());

  // Since we manually called release() on the MessageQueues, we cannot submit them back to the
  // factory
  EXPECT_CALL(
    logger,
    error(HasSubstr("expected to destroy 2 MessageQueue::Storage_t, but instead destroyed 0"), _))
    .Times(Exactly(1));
}
