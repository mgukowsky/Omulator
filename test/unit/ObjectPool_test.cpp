#include "omulator/util/ObjectPool.hpp"
#include "mocks/PrimitiveIOMock.hpp"

#include <gtest/gtest.h>
#include <map>
#include <utility>

namespace {
  struct MemBlock {
    unsigned char data[0x10];
  };
}

TEST(ObjectPool_test, getAnObj) {
  omulator::util::ObjectPool<MemBlock> op(4);
  MemBlock *i = op.get();
  i->data[0] = 0x12;
  EXPECT_EQ(i->data[0], 0x12) << "ObjectPool must return valid memory";
}

TEST(ObjectPool_test, returnToPool) {
  omulator::util::ObjectPool<MemBlock> op(4);
  MemBlock *i = op.get();
  MemBlock *i2 = op.get();
  i->data[0] = 0x12;
  i2->data[0] = 0x34;

  auto addrI = i;
  auto addrI2 = i2;

  op.return_to_pool(i);
  op.return_to_pool(i2);

  MemBlock *i2Retrieved = op.get();
  MemBlock *iRetrieved = op.get();

  EXPECT_EQ(i2Retrieved, addrI2) << "Items returned to the ObjectPool should be "
    "placed in a FIFO structure";
  EXPECT_EQ(iRetrieved, addrI) << "Items returned to the ObjectPool should be "
    "placed in a FIFO structure";
}

TEST(ObjectPool_test, growth) {
  omulator::util::ObjectPool<MemBlock> op(2);
  EXPECT_EQ(op.size(), 2) << "ObjectPools should be initialized to their requested size";

  [[maybe_unused]] MemBlock *m0 = op.get();
  [[maybe_unused]] MemBlock *m1 = op.get();

  EXPECT_EQ(op.size(), 2) << "Growth events should not occur until one more T element than "
    "available is requested";

  [[maybe_unused]] MemBlock *m2 = op.get();

  EXPECT_EQ(op.size(), 5) << "Growth events should not occur until one more T element than "
    "available is requested";

  [[maybe_unused]] MemBlock *m3 = op.get();
  [[maybe_unused]] MemBlock *m4 = op.get();

  EXPECT_EQ(op.size(), 5) << "Growth events should not occur until one more T element than "
    "available is requested";
  
  [[maybe_unused]] MemBlock *m5 = op.get();

  EXPECT_EQ(op.size(), 12) << "ObjectPool should grow by a factor of 1.5";
}

TEST(ObjectPool_test, stress) {
  omulator::util::ObjectPool<MemBlock> op(2);
  std::map<int, MemBlock*> memBlockMap;
  
  for(int i = 0; i < 1000; ++i) {
    memBlockMap.emplace(std::make_pair(i, op.get()));
  }

  for(const auto &entry : memBlockMap) {
    op.return_to_pool(entry.second);
  }

  for(int i = 999; i > -1; --i) {
    EXPECT_EQ(op.get(), memBlockMap[i]) << "Items returned to the ObjectPool should be "
      "placed in a FIFO structure";
  }
}

TEST(ObjectPool_test, returnForeignMemToPool) {
#ifndef NDEBUG
  ASSERT_DEATH({
    omulator::util::ObjectPool<MemBlock> op(4);
    MemBlock foreignMem;
    op.return_to_pool(&foreignMem);
  }, "") << "Memory from outside the ObjectPool should not be accepted by "
    "return_to_pool()";
#else
  ASSERT_EQ(1, 1);
#endif
}
