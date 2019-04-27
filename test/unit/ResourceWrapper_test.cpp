#include "mocks/Dummy.hpp"
#include "omulator/util/ResourceWrapper.hpp"

#include <gtest/gtest.h>

namespace {

int flag;

Dummy dummy_factory() {
  flag = 0x34;
  Dummy d;
  return d;
}

void dummy_dealloc([[maybe_unused]] Dummy d) noexcept {
  flag = 0x56;
}

void rdummy_dealloc([[maybe_unused]] Dummy &d) noexcept {
  flag = 0x78;
}

} /* namespace <anonymous> */

TEST(ResourceWrapper_test, passByValue) {
  Dummy::reset();
  flag = 0x12;

  EXPECT_EQ(flag, 0x12);

  {
    using DummyWrapper = omulator::util::ResourceWrapper<Dummy, dummy_factory, dummy_dealloc>;
    DummyWrapper dw;

    EXPECT_EQ(flag, 0x34);
  }

  EXPECT_EQ(flag, 0x56);
}

TEST(ResourceWrapper_test, passByRef) {
  Dummy::reset();
  flag = 0x12;

  EXPECT_EQ(flag, 0x12);

  {
    using DummyWrapper = omulator::util::ResourceWrapper<Dummy, dummy_factory, rdummy_dealloc>;
    DummyWrapper dw;

    EXPECT_EQ(flag, 0x34);
  }

  EXPECT_EQ(flag, 0x78);
}
