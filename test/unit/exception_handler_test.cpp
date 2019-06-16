#include <gtest/gtest.h>

#include "omulator/util/exception_handler.hpp"
#include "mocks/PrimitiveIOMock.hpp"

#include <new>
#include <exception>
#include <stdexcept>
#include <vector>

TEST(exception_handler, exceptionHandling) {
  ASSERT_DEATH({
    try {
      std::vector<int> v;
      [[maybe_unused]] auto x = v.at(1000);
    }
    catch (...) {
      omulator::util::exception_handler();
    };
  }, "An unexpected exception occurred") << "std::exception instances should be handled";
}

TEST(exception_handler, badAlloc) {
  ASSERT_DEATH({
    try {
      std::bad_alloc ba;
      throw ba;
    }
    catch(...) {
      omulator::util::exception_handler();
    };
  }, "Memory allocation failed") << "bad_alloc should be handled with a descriptive message";
}

TEST(exception_handler, unknownException) {
  ASSERT_DEATH({
    try {
      throw 7;
    }
    catch(...) {
      omulator::util::exception_handler();
    };
  }, "An unknown exception occurred") << "exceptions other than std::exception should be handled";
}

TEST(exception_handler, callWithoutException) {
  ASSERT_DEATH({
    omulator::util::exception_handler();
  }, "Global exception handler called without an active exception") 
    << "should alert the user when called without an exception";
}
