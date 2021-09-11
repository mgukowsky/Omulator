#include "omulator/msg/MessageBuffer.hpp"

#include "omulator/util/reinterpret.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>

using omulator::U16;
using omulator::U32;
using omulator::U8;
using omulator::msg::MessageBuffer;
using omulator::util::reinterpret;

namespace {
constexpr int MAGIC = 42;
}

TEST(MessageBuffer_test, simpleAlloc) {
  MessageBuffer mb = MessageBuffer::make_buff();

  EXPECT_TRUE(mb.empty()) << "MessageBuffers should be empty immediately after construction";

  constexpr U16 MSG_SIZ     = sizeof(U32);
  std::byte *   pAllocation = reinterpret_cast<std::byte *>(mb.alloc(MAGIC, MSG_SIZ));
  MessageBuffer::MessageHeader &header =
    reinterpret<MessageBuffer::MessageHeader>(pAllocation - MessageBuffer::HEADER_SIZE);

  EXPECT_FALSE(mb.empty())
    << "MessageBuffers should no longer be empty immediately following an allocation";

  *(reinterpret_cast<U32 *>(pAllocation)) = 0x1234'5678;

  std::byte *pAllocation2 = reinterpret_cast<std::byte *>(mb.alloc(MAGIC + 1, MSG_SIZ));
  MessageBuffer::MessageHeader &header2 =
    reinterpret<MessageBuffer::MessageHeader>(pAllocation2 - MessageBuffer::HEADER_SIZE);

  EXPECT_EQ(MAGIC, header.id)
    << "MessageBuffers should correctly record the ID of an allocated message";
  EXPECT_EQ(0x1234'5678, *(reinterpret_cast<U32 *>(pAllocation)))
    << "Headers for MessageBuffer allocations should not overwrite previously allocated memory "
       "within the message buffer";
  EXPECT_EQ(MAGIC + 1, header2.id)
    << "MessageBuffers should correctly record the ID of an allocated message";

  const std::ptrdiff_t allocDistance = std::abs(pAllocation2 - pAllocation);
  EXPECT_EQ(allocDistance, header.offsetNext)
    << "Allocations within MessageBuffers should point to the next message in the buffer, if one "
       "exists";
  EXPECT_EQ(MessageBuffer::HEADER_SIZE + MSG_SIZ, header2.offsetNext)
    << "The MessageHeader for the last allocation within a MessageBuffer should point to the "
       "memory directly after the allocation, where the next MessageHeader will be placed";
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
