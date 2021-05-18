#include "omulator/di/TypeMap.hpp"

#include <gtest/gtest.h>

class SimpleMock {
public:
  SimpleMock() { ++ctorCalls_; }
  ~SimpleMock() { ++dtorCalls_; }

  SimpleMock(const SimpleMock&) = delete;
  SimpleMock& operator=(const SimpleMock&) = delete;
  SimpleMock(SimpleMock&&) = delete;
  SimpleMock& operator=(SimpleMock&&) = delete;

  static int inline ctor_calls() noexcept { return ctorCalls_; }
  static inline int dtor_calls() noexcept { return dtorCalls_; }
  static inline void reset() noexcept { ctorCalls_ = dtorCalls_ = 0; }

private:
  inline static int ctorCalls_ = 0;
  inline static int dtorCalls_ = 0;
};

class TypeMapTest : public ::testing::Test {
protected:
  void SetUp() override {
    SimpleMock::reset();
  }
};

TEST_F(TypeMapTest, ctor_dtor_calls) {
  {
    omulator::di::TypeMap tm;
    tm.emplace<SimpleMock>();
    EXPECT_EQ(SimpleMock::ctor_calls(), 1)
      << "TypeMap::emplace<T>() should call T's ctor exactly once";
  }
  EXPECT_EQ(SimpleMock::dtor_calls(), 1)
    << "Destroying the type map should invoke T's dtor exactly once";
}

TEST_F(TypeMapTest, emplace) {
  omulator::di::TypeMap tm;
  EXPECT_FALSE(tm.has_key<int>())
    << "TypeMap should correctly report the status of types which have not been emplaced";
  EXPECT_FALSE(tm.emplace<int>(4)) << "TypeMap#emplace returns false when T does not yet exist";
  EXPECT_TRUE(tm.emplace<int>(5)) << "TypeMap#emplace returns true when T already exists";
  EXPECT_TRUE(tm.has_key<int>()) << "TypeMap#emplace should create an instance of T";
  EXPECT_EQ(tm.ref<int>(), 4) << "TypeMap#emplace should have no effect when T already exists";
}

TEST_F(TypeMapTest, ref) {
  omulator::di::TypeMap tm;
  tm.emplace<int>(4);
  EXPECT_EQ(tm.ref<int>(), 4) << "TypeMap#ref should return a reference to T when T exists";
  EXPECT_THROW(tm.ref<double>(), std::out_of_range)
    << "TypeMap#ref should throw std::out_of_range when T does not exist";
}

TEST_F(TypeMapTest, has_key) {
  omulator::di::TypeMap tm;
  tm.emplace<int>(4);
  EXPECT_TRUE(tm.has_key<int>()) << "TypeMap#has_key should return true when T exists";
  EXPECT_FALSE(tm.has_key<double>()) << "TypeMap#has_key should return false when T does not exist";
}
