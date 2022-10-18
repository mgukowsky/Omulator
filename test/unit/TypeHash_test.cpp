#include "omulator/util/TypeHash.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using omulator::util::TypeHash;

TEST(TypeHash_test, TypeHash_propreties) {
  EXPECT_EQ(TypeHash<int>, TypeHash<int>) << "identical types should have the same TypeHash";
  EXPECT_EQ(TypeHash<int>, TypeHash<const int>)
    << "CV modifiers should not produce different TypeHashes";
  EXPECT_NE(TypeHash<int>, TypeHash<int *>) << "pointers should produce different TypeHashes";
  EXPECT_EQ(TypeHash<int>, TypeHash<int &>) << "references should NOT produce different TypeHashes";
  EXPECT_EQ(TypeHash<std::vector<int>>, TypeHash<std::vector<int>>)
    << "identical template instantiations should have the same TypeHash";
  EXPECT_NE(TypeHash<std::vector<int>>, TypeHash<std::vector<std::string>>)
    << "different template instantiations should NOT have the same TypeHash";
}
