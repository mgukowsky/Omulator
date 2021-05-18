#include "omulator/di/TypeHash.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

TEST(TypeHash_test, TypeHash_propreties) {
  EXPECT_EQ(omulator::di::TypeHash<int>, omulator::di::TypeHash<int>) << "identical types should have the same TypeHash";
  EXPECT_EQ(omulator::di::TypeHash<int>, omulator::di::TypeHash<const int>) << "CV modifiers should not produce different TypeHashes";
  EXPECT_NE(omulator::di::TypeHash<int>, omulator::di::TypeHash<int *>) << "pointers should produce different TypeHashes";
  EXPECT_EQ(omulator::di::TypeHash<int>, omulator::di::TypeHash<int &>) << "references should NOT produce different TypeHashes";
  EXPECT_EQ(omulator::di::TypeHash<std::vector<int>>, omulator::di::TypeHash<std::vector<int>>)
    << "identical template instantiations should have the same TypeHash";
  EXPECT_NE(omulator::di::TypeHash<std::vector<int>>, omulator::di::TypeHash<std::vector<std::string>>)
    << "different template instantiations should NOT have the same TypeHash";
}
