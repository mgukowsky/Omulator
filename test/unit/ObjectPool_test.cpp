#include "omulator/util/ObjectPool.hpp"
#include "mocks/PrimitiveIOMock.hpp"

#include <gtest/gtest.h>

namespace {
  struct MemBlock {
    unsigned char data[0x10];
  };
}

TEST(ObjectPool_test, getAnObj) {
  omulator::util::ObjectPool<MemBlock> op(4);
  MemBlock &i = op.get();
  i.data[0] = 0x12;
  EXPECT_EQ(i.data[0], 0x12) << "ObjectPool must return valid memory";
}

TEST(ObjectPool_test, returnToPool) {
  omulator::util::ObjectPool<MemBlock> op(4);
  MemBlock &i = op.get();
  MemBlock &i2 = op.get();
  i.data[0] = 0x12;
  i2.data[0] = 0x34;

  auto addrI = &i;
  auto addrI2 = &i2;

  op.return_to_pool(i);
  op.return_to_pool(i2);

  MemBlock &i2Retrieved = op.get();
  MemBlock &iRetrieved = op.get();

  EXPECT_EQ(&i2Retrieved, addrI2) << "Items returned to the ObjectPool should be "
    "placed in a FIFO structure";
  EXPECT_EQ(&iRetrieved, addrI) << "Items returned to the ObjectPool should be "
    "placed in a FIFO structure";
}

TEST(ObjectPool_test, returnForeignMemToPool) {
#ifndef NDEBUG
  ASSERT_DEATH({
    omulator::util::ObjectPool<MemBlock> op(4);
    MemBlock foreignMem;
    op.return_to_pool(foreignMem);
  }, "") << "Memory from outside the ObjectPool should not be accepted by "
    "return_to_pool()";
#else
  ASSERT_EQ(1, 1);
#endif
}
