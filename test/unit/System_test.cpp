#include "omulator/System.hpp"

#include "mocks/LoggerMock.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

using omulator::Component;
using omulator::System;

TEST(System_test, simpleSystem) {
  LoggerMock                              logger;
  std::vector<std::unique_ptr<Component>> components;

  [[maybe_unused]] System system(logger, "testsystem", components);
}
