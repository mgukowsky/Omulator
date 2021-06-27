#include "omulator/di/Injector.hpp"

#include <gtest/gtest.h>

struct Klass {
  int               x;
  inline static int numCalls = 0;
  Klass() : x(42) { ++numCalls; }
};

class InjectorTest : public ::testing::Test {
protected:
  void SetUp() override { Klass::numCalls = 0; }
};

TEST_F(InjectorTest, defaultConstructibleTypes) {
  omulator::di::Injector injector;

  auto &i1 = injector.get<int>();
  auto &i2 = injector.get<int>();

  EXPECT_EQ(&i1, &i2)
    << "The injector should only return one instance of a default constructible type";

  auto &k1 = injector.get<Klass>();
  auto &k2 = injector.get<Klass>();

  EXPECT_EQ(&k1, &k2)
    << "The injector should only return one instance of a default constructible type";

  EXPECT_EQ(1, Klass::numCalls)
    << "The injector should only create one instance of a default constructible type";

  EXPECT_EQ(42, k1.x) << "The injector should only correctly instantiate one instance of a default "
                         "constructible type";
}
