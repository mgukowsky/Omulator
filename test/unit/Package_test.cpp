#include "omulator/msg/Package.hpp"

#include "omulator/di/TypeHash.hpp"
#include "omulator/msg/MessageBuffer.hpp"
#include "omulator/util/ObjectPool.hpp"
#include "omulator/util/reinterpret.hpp"

#include "mocks/PrimitiveIOMock.hpp"
#include "mocks/exception_handler_mock.hpp"

#include <gtest/gtest.h>

#include <cstddef>

using omulator::U32;
using omulator::msg::MessageBuffer;
using omulator::msg::Package;
using omulator::util::ObjectPool;
using omulator::util::reinterpret;

namespace {
constexpr U32 MAGIC = 0x1234'5678;
}

TEST(Package_test, templatedAlloc) {
  ObjectPool<MessageBuffer> pool(0x10);
  Package                   pkg(pool);

  int &buffInt = pkg.alloc<int>();
  buffInt      = MAGIC;

  std::byte *pRawAlloc = reinterpret_cast<std::byte *>(&buffInt);

  MessageBuffer::MessageHeader &header =
    reinterpret<MessageBuffer::MessageHeader>(pRawAlloc - MessageBuffer::HEADER_SIZE);
  EXPECT_EQ(omulator::di::TypeHash32<int>, header.id)
    << "Package's templated alloc function should correctly allocate storage for the desired type "
       "in the underlying message buffer";

  std::byte *pRawHeader = reinterpret_cast<std::byte *>(&header);

  EXPECT_EQ(MAGIC, reinterpret<U32>(pRawHeader + MessageBuffer::HEADER_SIZE))
    << "Package's templated alloc function should correctly allocate storage for the desired type "
       "in the underlying message buffer";
}
