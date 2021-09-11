#include "omulator/msg/MessageBuffer.hpp"

#include <gtest/gtest.h>

using omulator::U16;
using omulator::U32;
using omulator::U8;
using omulator::msg::MessageBuffer;

namespace {
constexpr int MAGIC = 42;
}

TEST(MessageBuffer_test, simpleAlloc) {
  MessageBuffer mb = MessageBuffer::make_buff();

  EXPECT_TRUE(mb.empty()) << "MessageBuffers should be empty immediately after construction";

  U8 *pAllocation = reinterpret_cast<U8 *>(mb.alloc(MAGIC, 8));
  EXPECT_FALSE(mb.empty())
    << "MessageBuffers should no longer be empty immediately following an allocation";

  *(reinterpret_cast<U32 *>(pAllocation)) = 0x1234'5678;

  U8 *pAllocation2 = reinterpret_cast<U8 *>(mb.alloc(MAGIC + 1, 8));

  EXPECT_EQ(MAGIC, *(reinterpret_cast<U16 *>(pAllocation - 8)))
    << "MessageBuffers should correctly record the ID of an allocated message";
  EXPECT_EQ(0x1234'5678, *(reinterpret_cast<U32 *>(pAllocation)))
    << "Headers for MessageBuffer allocations should not overwrite previously allocated memory "
       "within the message buffer";
  EXPECT_EQ(MAGIC + 1, *(reinterpret_cast<U16 *>(pAllocation2 - 8)))
    << "MessageBuffers should correctly record the ID of an allocated message";
}

TEST(MessageBuffer_test, maxAllocation) {
  MessageBuffer mb;
  EXPECT_THROW(mb.alloc(MAGIC, MessageBuffer::MAX_MSG_SIZE + 1), std::runtime_error)
    << "Attempting an allocation larger than MessageBuffer::MAX_MSG_SIZE should cause an exception "
       "to be raised.";

  MessageBuffer mb2 = MessageBuffer::make_buff();
  EXPECT_NE(nullptr, mb2.alloc(MAGIC, MessageBuffer::MAX_MSG_SIZE))
    << "An empty MessageBuffer should be able to allocate up to MessageBuffer::MAX_MSG_SIZE bytes";
  EXPECT_EQ(nullptr, mb2.alloc(MAGIC, 1))
    << "MessageBuffer::alloc should return a nullptr when the MessageBuffer cannot accomodate an "
       "allocation of the requested number of bytes";
}
