#pragma once

#include "omulator/ILogger.hpp"

#include <gmock/gmock.h>

class LoggerMockKlass : public omulator::ILogger {
public:
  MOCK_METHOD(void, critical, (const char * const, omulator::util::SourceLocation), (override));
  MOCK_METHOD(void, error, (const char * const, omulator::util::SourceLocation), (override));
  MOCK_METHOD(void, warn, (const char * const, omulator::util::SourceLocation), (override));
  MOCK_METHOD(void, info, (const char * const, omulator::util::SourceLocation), (override));
  MOCK_METHOD(void, debug, (const char * const, omulator::util::SourceLocation), (override));
  MOCK_METHOD(void, trace, (const char * const, omulator::util::SourceLocation), (override));
  MOCK_METHOD(void, set_level, (LogLevel level), (override));
};

// Consider any mock call not caught via an EXPECT_* function to be an error
using LoggerMock = ::testing::StrictMock<LoggerMockKlass>;
