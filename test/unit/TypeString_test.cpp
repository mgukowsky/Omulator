#include "omulator/util/TypeString.hpp"

#include <gtest/gtest.h>

#include <string>

class Klass { };

using omulator::util::TypeString;

TEST(TypeString_test, getTypeStrings) {
  // In an interesting quirk of the standard, I learned here not to call data() on a string_view
  // returned by string_view#substr(pos, len), because while the string_view substr itself will
  // truncate correctly at len chars after pos, the underlying char* returned by string_view#data
  // will simply be pointing at pos, with no knowledge of len! The result of this is that the
  // c_string will simply end at the null terminator of the underlying const char *!
  EXPECT_STREQ("int", std::string(TypeString<int>).c_str())
    << "TypeString<T> correctly generates a string with the name of T";
  EXPECT_STREQ("int", std::string(TypeString<int &>).c_str())
    << "TypeString<T> correctly generates a string with the name of T";
  EXPECT_STREQ("const int", std::string(TypeString<const int * const>).c_str())
    << "TypeString<T> correctly generates a string with the name of T";
  EXPECT_STREQ("void()", std::string(TypeString<void (*)()>).c_str())
    << "TypeString<T> correctly generates a string with the name of T";
  EXPECT_STREQ("Klass", std::string(TypeString<Klass>).c_str())
    << "TypeString<T> correctly generates a string with the name of T";
  EXPECT_STREQ("std::vector<const Klass>",
               std::string(TypeString<const std::vector<const Klass>>).c_str())
    << "TypeString<T> correctly generates a string with the name of T";
}
