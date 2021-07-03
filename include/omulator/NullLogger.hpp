#pragma once

#include "omulator/ILogger.hpp"

namespace omulator {
class NullLogger : public ILogger {
public:
  ~NullLogger() override = default;

  void critical([[maybe_unused]] const char * const msg) { }
  void error([[maybe_unused]] const char * const msg) { }
  void warn([[maybe_unused]] const char * const msg) { }
  void info([[maybe_unused]] const char * const msg) { }
  void debug([[maybe_unused]] const char * const msg) { }
  void trace([[maybe_unused]] const char * const msg) { }

  void set_level([[maybe_unused]] LogLevel level) { }
};
}  // namespace omulator
