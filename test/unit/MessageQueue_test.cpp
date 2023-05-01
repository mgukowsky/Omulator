#include "omulator/msg/MessageQueue.hpp"

#include "omulator/di/TypeMap.hpp"
#include "omulator/util/to_underlying.hpp"

#include "mocks/LoggerMock.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <utility>
#include <vector>

using ::testing::_;
using ::testing::Exactly;
using ::testing::HasSubstr;

using omulator::U64;
using omulator::msg::Message;
using omulator::msg::MessageQueue;
using omulator::msg::MessageType;
using omulator::util::to_underlying;

namespace {
U64 aDtorCount = 0;

struct A {
  A() : val(0) { }

  ~A() { ++aDtorCount; }

  int val;
};

}  // namespace

TEST(MessageQueue_test, msgPayloadFieldSizeCheck) {
  EXPECT_TRUE(sizeof(Message::payload) >= sizeof(void *))
    << "Message::payload must be at least large enough to store a pointer";
}

TEST(MessageQueue_test, singleThreadSendRecv) {
  LoggerMock              logger;
  MessageQueue::Storage_t storage(0);
  MessageQueue            mq(&storage, logger);

  mq.push(MessageType::MSG_NULL, 0);

  mq.push(MessageType::DEMO_MSG_A, 42);

  mq.seal();

  U64 i = 0;
  mq.pump_msgs([&](const Message &msg) {
    if(msg.type == MessageType::DEMO_MSG_A) {
      i = msg.payload;
    }
  });

  EXPECT_EQ(42, i)
    << "MessageQueue::pump_msgs should invoke the provided callback for each message in the queue";
}

TEST(MessageQueue_test, sealUnseal) {
  LoggerMock              logger;
  MessageQueue::Storage_t storage(0);
  MessageQueue            mq(&storage, logger);

  mq.push(MessageType::DEMO_MSG_A, 42);

  U64 i = 0;
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
  LoggerMock              logger;
  MessageQueue::Storage_t storage(0);
  MessageQueue            mq(&storage, logger);

  std::atomic_int  flag = 0;
  std::vector<U64> vals;

  // When not provided by the caller, the payload should default to zero
  mq.push(MessageType::DEMO_MSG_A);
  mq.push(MessageType::DEMO_MSG_A, 1);
  mq.push(MessageType::DEMO_MSG_A, 2);

  // push() should accepts types that can be converted to U64
  unsigned char charVal = 3;
  mq.push(MessageType::DEMO_MSG_A, charVal);

  int  intVal = 0;
  int *pInt   = &intVal;
  mq.push(MessageType::DEMO_MSG_B, pInt);

  mq.push(MessageType::DEMO_MSG_C);

  mq.seal();

  // A small example of how to send a message to one thread...
  std::jthread jthrd([&]() {
    mq.pump_msgs([&](const Message &msg) {
      if(msg.type == MessageType::DEMO_MSG_A) {
        vals.push_back(msg.payload);
      }
      else if(msg.type == MessageType::DEMO_MSG_B) {
        *(reinterpret_cast<int *>(msg.payload)) = 1234;
      }
      else if(msg.type == MessageType::DEMO_MSG_C) {
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

  EXPECT_EQ(1234, intVal)
    << "MessageQueues should appropriately respond to all messages in their queue";
}

TEST(MessageQueue_test, managedPayloads) {
  aDtorCount = 0;

  LoggerMock              logger;
  MessageQueue::Storage_t storage(0);
  MessageQueue            mq(&storage, logger);

  constexpr int MAGIC = 7;

  A &ra  = mq.push_managed_payload<A>(MessageType::DEMO_MSG_A);
  ra.val = MAGIC;

  mq.seal();
  mq.pump_msgs([&]([[maybe_unused]] const Message &msg) {
    if(msg.type == MessageType::DEMO_MSG_A) {
      const A &a = msg.get_managed_payload<A>();

      EXPECT_EQ(MAGIC, a.val)
        << "MessageQueue::push_managed_payload should return a reference that can be used to "
           "manipulate the emplaced instance";
    }
  });
  EXPECT_EQ(1, aDtorCount)
    << "Messages with managed payloads should be properly cleaned up; unmanaged "
       "payloads (even if the payload is a pointer) should not be cleaned up by the MessageQueue";

  A *paUnmanaged = new A;

  storage.storage.clear();
  MessageQueue mq2(&storage, logger);
  mq2.push(MessageType::DEMO_MSG_A, paUnmanaged);
  mq2.seal();
  mq2.pump_msgs([&]([[maybe_unused]] const Message &msg) {});

  EXPECT_EQ(1, aDtorCount) << "Messages with unmanaged payloads (even if the payloads are "
                              "pointers) should not be clean up by the MessageQueue";

  delete paUnmanaged;
}

// Make sure that we get a warning when we drop messages with types > MessageType::MSG_MAX
TEST(MessageQueue_test, msgMaxTest) {
  LoggerMock              logger;
  MessageQueue::Storage_t storage(0);
  MessageQueue            mq(&storage, logger);

  mq.push(static_cast<MessageType>(to_underlying(MessageType::MSG_MAX) + 1));
  mq.seal();

  EXPECT_CALL(logger,
              error(HasSubstr("Message with type exceeding MSG_MAX detected by "
                              "MessageQueue::pump_msgs; this message will be dropped"),
                    _))
    .Times(Exactly(1));
  mq.pump_msgs([]([[maybe_unused]] const Message &msg) {});
}

TEST(MessageQueue_test, validityCheckTests) {
  LoggerMock              logger;
  MessageQueue::Storage_t storage(0);
  MessageQueue            mq(&storage, logger);

  mq.mark_invalid();

  EXPECT_CALL(
    logger,
    error(HasSubstr("Attempted to call MessageQueue::push() on a MessageQueue that is not valid"),
          _))
    .Times(Exactly(1));
  mq.push(MessageType::DEMO_MSG_A);

  EXPECT_CALL(
    logger,
    error(
      HasSubstr("Attempted to call MessageQueue::pump_msgs() on a MessageQueue that is not valid"),
      _))
    .Times(Exactly(1));
  mq.pump_msgs([]([[maybe_unused]] const Message &msg) {});

  EXPECT_THROW(mq.push_managed_payload<int>(MessageType::DEMO_MSG_A), std::runtime_error)
    << "MessageQueue::push_managed_payload() should throw when called on an invalid MessageQueue";
}

TEST(MessageQueue_test, moveCtorTest) {
  LoggerMock              logger;
  MessageQueue::Storage_t storage(0);
  MessageQueue            mq(&storage, logger);

  MessageQueue mq1(std::move(mq));

  EXPECT_TRUE(mq1.valid())
    << "MessageQueue's move constructors should mark moved-from instances as invalid";
  EXPECT_FALSE(mq.valid())
    << "MessageQueue's move constructors should mark moved-from instances as invalid";

  MessageQueue mq2 = std::move(mq1);
  EXPECT_TRUE(mq2.valid())
    << "MessageQueue's move constructors should mark moved-from instances as invalid";
  EXPECT_FALSE(mq1.valid())
    << "MessageQueue's move constructors should mark moved-from instances as invalid";

  mq2.mark_invalid();

  MessageQueue mq3(std::move(mq2));
  EXPECT_FALSE(mq3.valid()) << "MessageQueue's move constructors should correctly move instances "
                               "which have been marked invalid";
  EXPECT_FALSE(mq2.valid()) << "MessageQueue's move constructors should correctly move instances "
                               "which have been marked invalid";
}

TEST(MessageQueue_test, clear) {
  aDtorCount = 0;

  LoggerMock              logger;
  MessageQueue::Storage_t storage(0);
  MessageQueue            mq(&storage, logger);

  mq.push_managed_payload<A>(MessageType::DEMO_MSG_A);
  mq.push_managed_payload<A>(MessageType::DEMO_MSG_B);

  for(const Message &msg : storage.storage) {
    EXPECT_TRUE(msg.has_managed_payload())
      << "MessageQueue::push_managed_payload() should mark the pushed message as managed";
    EXPECT_NE(0, msg.payload) << "MessageQueue::push_managed_payload should add a message with a "
                                 "pointer to the managed payload";
  }

  EXPECT_EQ(0, aDtorCount);
  EXPECT_FALSE(mq.sealed());

  mq.clear();

  for(const Message &msg : storage.storage) {
    EXPECT_TRUE(msg.has_managed_payload());
    EXPECT_EQ(0, msg.payload)
      << "MessageQueue::clear should set the payload of each message with a managed payload to 0";
  }

  EXPECT_EQ(2, aDtorCount)
    << "MessageQueue::clear should properly destroy any managed payloads in the MessageQueue";
  EXPECT_TRUE(mq.sealed()) << "MessageQueue::clear should seal the queue";

  mq.clear();
  EXPECT_EQ(2, aDtorCount) << "Subsequent calls MessageQueue::clear on the same MessageQueue "
                              "instance should have no effect";
}
