#pragma once

#include "omulator/ILogger.hpp"

namespace omulator {
class NullLogger : public ILogger {
public:
  ~NullLogger() override = default;

  void critical(
    [[maybe_unused]] const char * const         msg,
    [[maybe_unused]] const util::SourceLocation location = util::SourceLocation::current()) { }
  void error(
    [[maybe_unused]] const char * const         msg,
    [[maybe_unused]] const util::SourceLocation location = util::SourceLocation::current()) { }
  void warn(
    [[maybe_unused]] const char * const         msg,
    [[maybe_unused]] const util::SourceLocation location = util::SourceLocation::current()) { }
  void info(
    [[maybe_unused]] const char * const         msg,
    [[maybe_unused]] const util::SourceLocation location = util::SourceLocation::current()) { }
  void debug(
    [[maybe_unused]] const char * const         msg,
    [[maybe_unused]] const util::SourceLocation location = util::SourceLocation::current()) { }
  void trace(
    [[maybe_unused]] const char * const         msg,
    [[maybe_unused]] const util::SourceLocation location = util::SourceLocation::current()) { }

  void set_level([[maybe_unused]] LogLevel level) { }
};
}  // namespace omulator
