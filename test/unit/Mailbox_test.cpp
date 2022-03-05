#include "omulator/msg/Mailbox.hpp"

#include "omulator/di/TypeHash.hpp"
#include "omulator/oml_types.hpp"

#include "mocks/LoggerMock.hpp"
#include "mocks/PrimitiveIOMock.hpp"
#include "mocks/exception_handler_mock.hpp"

#include <gtest/gtest.h>

#include <array>
#include <memory>

using omulator::U16;
using omulator::U32;
using omulator::U64;
using omulator::di::TypeHash32;
using omulator::msg::Mailbox;
using omulator::msg::MessageBuffer;
using omulator::msg::Package;
using omulator::msg::ReceiverMap_t;
using omulator::util::ObjectPool;

namespace {
std::unique_ptr<LoggerMock>                pLogger;
std::unique_ptr<ObjectPool<MessageBuffer>> pMbPool;
std::unique_ptr<ObjectPool<Package>>       pPkgPool;

constexpr U32 MAGIC = 0xABCD;
}  // namespace

class Mailbox_test : public ::testing::Test {
protected:
  void SetUp() override {
    pLogger.reset(new LoggerMock);
    pMbPool.reset(new ObjectPool<MessageBuffer>(0x10));
    pPkgPool.reset(new ObjectPool<Package>(0x10));
  }

  void TearDown() override {
    // We _HAVE_ to delete the logger mock here, as mock objects need to be destroyed
    // after tests end and before GoogleMock begins its global teardown, otherwise we
    // will get an exception thrown. See https://github.com/google/googletest/issues/1963
    pLogger.reset();
    pMbPool.reset();
    pPkgPool.reset();
  }
};

TEST_F(Mailbox_test, packageLifecycle) {
  Mailbox              mailbox(*pLogger, *pPkgPool, *pMbPool);
  U32                  i = 0;
  U64                  j = 0;
  [[maybe_unused]] U16 k = 0;

  Package *pkg = mailbox.open_pkg();
  pkg->alloc_msg(TypeHash32<U32>);
  pkg->alloc_msg(TypeHash32<U64>);

  mailbox.on(TypeHash32<U32>, [&](const void *) { i = MAGIC; });
  mailbox.on(TypeHash32<U64>, [&](const void *) { j = MAGIC + 1; });

  mailbox.recv();
  EXPECT_EQ(0, i) << "Calling Mailbox::recv() when no packages have been sent should result in no "
                     "messages being processed";
  EXPECT_EQ(0, j) << "Calling Mailbox::recv() when no packages have been sent should result in no "
                     "messages being processed";

  mailbox.send(pkg);
  mailbox.recv();
  EXPECT_EQ(MAGIC, i) << "Mailbox::recv() should properly process all messages that have "
                         "been submitted up to that point";
  EXPECT_EQ(MAGIC + 1, j) << "Mailbox::recv() should properly process all messages that "
                             "have been submitted up to that point";

// ObjectPool only checks for memory from a different pool in debug builds
#ifndef NDEBUG
  Mailbox mailbox2(*pLogger, *pPkgPool, *pMbPool);
  mailbox2.on(TypeHash32<U16>, [&](const void *) { k = MAGIC + 2; });

  // N.B. we are creating a Package from one Mailbox and sending it to another.
  Package *pkg2 = mailbox.open_pkg();
  pkg2->alloc_msg(TypeHash32<U16>);
  mailbox2.send(pkg2);
  mailbox2.recv();
  EXPECT_EQ(MAGIC + 2, k) << "Sending a Package to a Mailbox other than the one it was opened in "
                             "should not cause an error so long as the Packages were created with "
                             "the same instances of each of their dependencies";
#endif
}

// N.B. this tests also checks for the ability for the underlying ObjectPool's to expand several
// times
TEST_F(Mailbox_test, stress) {
  constexpr std::size_t ARRSIZ = 1000;
  Mailbox               mailbox(*pLogger, *pPkgPool, *pMbPool);

  std::array<U64, ARRSIZ> arr{0};

  for(std::size_t i = 0; i < ARRSIZ; ++i) {
    Package *pkg = mailbox.open_pkg();
    pkg->alloc_msg(TypeHash32<U64>);
    mailbox.send(pkg);
  }

  std::size_t idx = 0;
  mailbox.on(TypeHash32<U64>, [&](const void *) {
    arr[idx] = MAGIC + idx;
    ++idx;
  });
  mailbox.recv();

  for(std::size_t i = 0; i < ARRSIZ; ++i) {
    EXPECT_EQ(MAGIC + i, arr[i]);
  }
}

TEST_F(Mailbox_test, on_off_msgs) {
  Mailbox mailbox(*pLogger, *pPkgPool, *pMbPool);

  constexpr int MSG = 42;
  int           i   = 0;

  mailbox.on(MSG, [&](const void *) { ++i; });

  Package *pkg = mailbox.open_pkg();
  pkg->alloc_msg(MSG);
  mailbox.send(pkg);
  mailbox.recv();
  EXPECT_EQ(1, i) << "Mailbox::on should enable callbacks for a given message";

  pkg = mailbox.open_pkg();
  pkg->alloc_msg(MSG);
  mailbox.send(pkg);
  mailbox.recv();
  EXPECT_EQ(2, i) << "Mailbox::on should enable callbacks for a given message";

  mailbox.off(MSG);
  pkg = mailbox.open_pkg();
  pkg->alloc_msg(MSG);
  mailbox.send(pkg);
  mailbox.recv();
  EXPECT_EQ(2, i) << "Mailbox::off should disable callbacks for a given message";

  mailbox.on(MSG, [&](const void *) { ++i; });
  pkg = mailbox.open_pkg();
  pkg->alloc_msg(MSG);
  mailbox.send(pkg);
  mailbox.recv();
  EXPECT_EQ(3, i)
    << "Mailbox::on should have the ability to re-enable callbacks for a given message";

  EXPECT_CALL(*pLogger, warn).Times(1);
  mailbox.on(MSG, [&](const void *) { --i; });
  pkg = mailbox.open_pkg();
  pkg->alloc_msg(MSG);
  mailbox.send(pkg);
  mailbox.recv();
  EXPECT_EQ(4, i) << "Mailbox::on should not replace a callback for a given message unless "
                     "Mailbox::off is called first";
}
