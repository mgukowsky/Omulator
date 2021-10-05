#pragma once

#include "omulator/ILogger.hpp"

#include <gmock/gmock.h>

class LoggerMock : public omulator::ILogger {
public:
  MOCK_METHOD(void, critical, (const char * const), (override));
  MOCK_METHOD(void, error, (const char * const), (override));
  MOCK_METHOD(void, warn, (const char * const), (override));
  MOCK_METHOD(void, info, (const char * const), (override));
  MOCK_METHOD(void, debug, (const char * const), (override));
  MOCK_METHOD(void, trace, (const char * const), (override));
  MOCK_METHOD(void, set_level, (LogLevel level), (override));
};
