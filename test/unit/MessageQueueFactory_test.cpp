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

  MessageQueue *pmq = mqf.get();
  EXPECT_NE(nullptr, pmq) << "MessageQueueFactory::get() should never return a nullptr";

  mqf.submit(pmq);
  MessageQueue *pmq2 = mqf.get();
  EXPECT_EQ(pmq, pmq2)
    << "MessageQueueFactory should reuse MessageQueues that have been submitted back to it";

  mqf.submit(pmq2);

  // Shouldn't warn about memory leaks since we have called submit() for each MessageQueue acquired
  // through get()
  EXPECT_CALL(logger, error).Times(Exactly(0));
}

TEST(MessageQueue_test, memLeakWarn) {
  LoggerMock          logger;
  MessageQueueFactory mqf(logger);

  MessageQueue                  *pmq1 = mqf.get();
  [[maybe_unused]] MessageQueue *pmq2 = mqf.get();

  mqf.submit(pmq1);
  EXPECT_CALL(logger, error).Times(Exactly(1));
}
