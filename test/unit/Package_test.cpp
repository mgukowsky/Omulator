#include "omulator/msg/Package.hpp"

#include "omulator/di/TypeHash.hpp"
#include "omulator/msg/MessageBuffer.hpp"
#include "omulator/util/ObjectPool.hpp"
#include "omulator/util/ResourceWrapper.hpp"
#include "omulator/util/reinterpret.hpp"

#include "mocks/LoggerMock.hpp"
#include "mocks/PrimitiveIOMock.hpp"
#include "mocks/exception_handler_mock.hpp"

#include <gtest/gtest.h>

#include <cstddef>

using ::testing::Exactly;

using omulator::U32;
using omulator::di::TypeHash32;
using omulator::msg::MessageBuffer;
using omulator::msg::Package;
using omulator::util::ObjectPool;
using omulator::util::reinterpret;

namespace {
constexpr U32 MAGIC = 0x1234'5678;

Package FnPkgAlloc(ObjectPool<MessageBuffer> *pool, LoggerMock *logger) {
  Package pkg;
  pkg.reset(pool, logger);
  return pkg;
};

void FnPkgDealloc(Package &pkg) noexcept { pkg.release(); };

using ManagedPkg_t = omulator::util::ResourceWrapper<Package, FnPkgAlloc, FnPkgDealloc>;

}  // namespace

TEST(Package_test, simpleAlloc) {
  LoggerMock                logger;
  ObjectPool<MessageBuffer> pool(0x10);

  ManagedPkg_t mpkg(&pool, &logger);
  Package     &pkg = *mpkg;

  int &buffInt = pkg.alloc_data<int>();
  buffInt      = MAGIC;

  std::byte *pRawAlloc = reinterpret_cast<std::byte *>(&buffInt);

  MessageBuffer::MessageHeader &header =
    reinterpret<MessageBuffer::MessageHeader>(pRawAlloc - MessageBuffer::HEADER_SIZE);
  EXPECT_EQ(omulator::di::TypeHash32<int>, header.id)
    << "Package's templated alloc_data function should correctly allocate storage for the desired "
       "type in the underlying message buffer";

  std::byte *pRawHeader = reinterpret_cast<std::byte *>(&header);

  EXPECT_EQ(MAGIC, reinterpret<U32>(pRawHeader + MessageBuffer::HEADER_SIZE))
    << "Package's templated alloc_data function should correctly allocate storage for the desired "
       "type in the underlying message buffer";
}

TEST(Package_test, allocMsg) {
  LoggerMock                logger;
  ObjectPool<MessageBuffer> pool(0x10);

  ManagedPkg_t mpkg(&pool, &logger);
  Package     &pkg = *mpkg;

  pkg.alloc_msg(MAGIC);

  {
    // Since we are providing no receivers, we should log that we dropped the since message in the
    // Package
    EXPECT_CALL(logger, warn).Times(Exactly(1));
    pkg.receive_msgs({});
  }

  constexpr int B33F = 0xB33F;
  int           i    = 0;
  pkg.receive_msgs({
    {MAGIC, [&](const void *) { i = B33F; }}
  });

  EXPECT_EQ(B33F, i)
    << "Packages should correctly invoke a receiver function for a given message ID "
       "when that ID is received as part of a package";
}

TEST(Package_test, allocData) {
  LoggerMock                logger;
  ObjectPool<MessageBuffer> pool(0x10);

  ManagedPkg_t mpkg(&pool, &logger);
  Package     &pkg = *mpkg;

  struct MaxMsg {
    std::byte data[MessageBuffer::MAX_MSG_SIZE];
  };

  constexpr std::byte VAL{0b10101010};
  constexpr std::byte FLIPVAL{VAL ^ std::byte{0xFF}};

  MaxMsg &msga = pkg.alloc_data<MaxMsg>();
  MaxMsg &msgb = pkg.alloc_data<MaxMsg>();

  for(std::size_t i = 0; i < MessageBuffer::MAX_MSG_SIZE; ++i) {
    msga.data[i] = VAL;
    msgb.data[i] = FLIPVAL;
  }

  bool shouldFlip = false;

  pkg.receive_msgs({
    {TypeHash32<MaxMsg>, [&](const void *data) {
       const MaxMsg &msg = reinterpret<const MaxMsg>(data);

       std::byte comp = shouldFlip ? FLIPVAL : VAL;

       for(std::size_t i = 0; i < MessageBuffer::MAX_MSG_SIZE; ++i) {
         EXPECT_EQ(comp, msg.data[i])
           << "Data set within a Package should be properly received by clients";
       }

       shouldFlip = true;
     }}
  });
}
