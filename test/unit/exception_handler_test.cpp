#include "omulator/util/exception_handler.hpp"

#include "mocks/PrimitiveIOMock.hpp"

#include <gtest/gtest.h>

#include <exception>
#include <new>
#include <stdexcept>
#include <vector>

class exception_handler_test : public ::testing::Test {
public:
  exception_handler_test() {
#if defined(OML_COMPILER_MSVC) || defined(OML_COMPILER_CLANG_CL)
// Prevent death tests from returning false negatives in Visual Studio
    _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, [](int, char*, int*) -> int { return true; });
#endif
  }
};

TEST_F(exception_handler_test, exceptionHandling) {
  ASSERT_DEATH(
    {
      try {
        std::vector<int>      v;
        [[maybe_unused]] auto x = v.at(1000);
      }
      catch(...) {
        omulator::util::exception_handler();
      };
    },
    "An unexpected exception occurred")
    << "std::exception instances should be handled";
}

TEST_F(exception_handler_test, badAlloc) {
  ASSERT_DEATH(
    {
      try {
        std::bad_alloc ba;
        throw ba;
      }
      catch(...) {
        omulator::util::exception_handler();
      };
    },
    "Memory allocation failed")
    << "bad_alloc should be handled with a descriptive message";
}

TEST_F(exception_handler_test, unknownException) {
  ASSERT_DEATH(
    {
      try {
        throw 7;
      }
      catch(...) {
        omulator::util::exception_handler();
      };
    },
    "An unknown exception occurred")
    << "exceptions other than std::exception should be handled";
}

TEST_F(exception_handler_test, callWithoutException) {
  ASSERT_DEATH({ omulator::util::exception_handler(); },
               "Global exception handler called without an active exception")
    << "should alert the user when called without an exception";
}
