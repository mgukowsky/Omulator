#include "omulator/msg/MessageQueue.hpp"

#include "mocks/LoggerMock.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

using omulator::U64;
using omulator::msg::Message;
using omulator::msg::MessageQueue;
using omulator::msg::MessageType;

using ::testing::Exactly;

TEST(MessageQueue_test, singleThreadSendRecv) {
  LoggerMock   logger;
  MessageQueue mq(logger);

  EXPECT_EQ(0, mq.num_null_msgs())
    << "A message queue's number of null messages should be initialized to zero";

  mq.push(MessageType::MSG_NULL, 0);

  mq.push(MessageType::DEMO_MSG_A, 42);

  mq.seal();

  U64 i = 0;
  mq.pump_msgs([&](const Message &msg) {
    if(msg.type == MessageType::DEMO_MSG_A) {
      i = msg.payload;
    }
  });

  EXPECT_EQ(1, mq.num_null_msgs()) << "A message queue's number of null messages should be "
                                      "incremented each time a MSG_NULL message is processed";
  EXPECT_EQ(42, i)
    << "MessageQueue::pump_msgs should invoke the provided callback for each message in the queue";

  mq.reset();
  i = 0;
  EXPECT_EQ(0, mq.num_null_msgs()) << "A message queue's number of null messages should be set to "
                                      "zero when MessageQueue::reset() is invoked";

  mq.seal();

  mq.pump_msgs([&](const Message &msg) {
    if(msg.type == MessageType::DEMO_MSG_A) {
      i = msg.payload;
    }
  });
  EXPECT_EQ(0, mq.num_null_msgs())
    << "Calling MessageQueue::reset() should remove any messages currently in the queue";
  EXPECT_EQ(0, i)
    << "Calling MessageQueue::reset() should remove any messages currently in the queue";
}

TEST(MessageQueue_test, sealUnseal) {
  LoggerMock   logger;
  MessageQueue mq(logger);

  mq.push(MessageType::DEMO_MSG_A, 42);

  int i = 0;
  // Should not be able to pump messages on a queue that is not sealed
  EXPECT_CALL(logger, error).Times(Exactly(1));
  mq.pump_msgs([&](const Message &msg) {
    if(msg.type == MessageType::DEMO_MSG_A) {
      i = msg.payload;
    }
  });

  EXPECT_EQ(0, i)
    << "MessageQueue::pump_msgs() should not process any messages from a queue that is not sealed";

  mq.seal();

  // Should not be able to push messages onto a sealed queue
  EXPECT_CALL(logger, error).Times(Exactly(1));
  mq.push(MessageType::DEMO_MSG_A, 43);

  mq.pump_msgs([&](const Message &msg) {
    if(msg.type == MessageType::DEMO_MSG_A) {
      i = msg.payload;
    }
  });
  EXPECT_EQ(42, i) << "MessageQueue::push() should have no effect on a sealed queue";
}

TEST(MessageQueue_test, multipleMsgTypes) {
  LoggerMock   logger;
  MessageQueue mq(logger);

  std::atomic_int  flag = 0;
  std::vector<U64> vals;

  // When not provided by the caller, the payload should default to zero
  mq.push(MessageType::DEMO_MSG_A);
  mq.push(MessageType::DEMO_MSG_A, 1);
  mq.push(MessageType::DEMO_MSG_A, 2);
  mq.push(MessageType::DEMO_MSG_A, 3);

  mq.push(MessageType::DEMO_MSG_B);

  mq.seal();

  // A small example of how to send a message to one thread...
  std::jthread jthrd([&]() {
    mq.pump_msgs([&](const Message &msg) {
      if(msg.type == MessageType::DEMO_MSG_A) {
        vals.push_back(msg.payload);
      }
      else if(msg.type == MessageType::DEMO_MSG_B) {
        flag = 1;
        // ...and have the thread send a response back to the other thread
        flag.notify_all();
      }
    });
  });

  flag.wait(0);

  int i = 0;
  for(const U64 val : vals) {
    EXPECT_EQ(i, val)
      << "MessageQueues should appropriately respond to all messages in their queue";
    ++i;
  }
}
