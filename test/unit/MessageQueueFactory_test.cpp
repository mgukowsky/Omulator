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

using ::testing::Exactly;

TEST(MessageQueue_test, getAndSubmit) {
  LoggerMock          logger;
  MessageQueueFactory mqf(logger);

  MessageQueue mq = mqf.get();

  mqf.submit(mq);
  MessageQueue recycledStorageMQ = mqf.get();
  MessageQueue freshStorageMQ    = mqf.get();

  // Test for storage reuse via the reset() method, which returns a pointer to the MQ's internal
  // storage.
  EXPECT_EQ(mq.reset(), recycledStorageMQ.reset())
    << "MessageQueueFactory should reuse MessageQueue::Storage_t "
       "instances that have been submitted back to it";

  EXPECT_NE(mq.reset(), freshStorageMQ.reset())
    << "MessageQueueFactory should create a new instance of MessageQueue::Storage_t "
       "if no instances have been submitted back to it";

  // N.B. that what follows is only for testing purposes; MessageQueue instances should never be
  // referenced once they are submitted back into a MessageQueueFactory
  recycledStorageMQ.seal();
  EXPECT_TRUE(recycledStorageMQ.sealed())
    << "MessageQueueFactory::submit() should call MessageQueue::reset()";
  mqf.submit(recycledStorageMQ);
  EXPECT_FALSE(recycledStorageMQ.sealed())
    << "MessageQueueFactory::submit() should call MessageQueue::reset()";

  mqf.submit(freshStorageMQ);

  // Shouldn't warn about memory leaks since we have called submit() for each MessageQueue acquired
  // through get()
  EXPECT_CALL(logger, error).Times(Exactly(0));
}

TEST(MessageQueue_test, memLeakWarn) {
  LoggerMock          logger;
  MessageQueueFactory mqf(logger);

  MessageQueue                  mq1 = mqf.get();
  [[maybe_unused]] MessageQueue mq2 = mqf.get();

  mqf.submit(mq1);
  EXPECT_CALL(logger, error).Times(Exactly(1));
}
