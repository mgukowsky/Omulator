#pragma once

#include <spdlog/spdlog.h>

#include <string>
#include <utility>

namespace omulator {

/**
 * Logging class to abstract away the underlying log library. All logging
 * functions accept a libfmt-style series of arguments, ie. log("fmt string", args...)
 */

class Logger {
public:
  enum class LogLevel {
    OFF,
    CRITICAL,
    ERR,
    WARN,
    INFO,
    DEBUG,
    TRACE
  };

  // N.B. that "%+" is spdlog's default format
  Logger(const omulator::Logger::LogLevel initialLevel,
    const std::string &pattern = "%+");
  ~Logger();

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;
  Logger(Logger&&) = delete;
  Logger& operator=(Logger&&) = delete;

  //TODO: OK for all Logging functions to be noexcept?
  template<typename ...Args>
  inline void critical(Args &&...args) const noexcept {
    spdlog::critical(std::forward<Args>(args)...);
  }

  template<typename ...Args>
  inline void error(Args &&...args) const noexcept {
    spdlog::error(std::forward<Args>(args)...);
  }

  template<typename ...Args>
  inline void warn(Args &&...args) const noexcept {
    spdlog::warn(std::forward<Args>(args)...);
  }

  template<typename ...Args>
  inline void info(Args &&...args) const noexcept {
    spdlog::info(std::forward<Args>(args)...);
  }

  template<typename ...Args>
  inline void debug(Args &&...args) const noexcept {
    spdlog::debug(std::forward<Args>(args)...);
  }

  template<typename ...Args>
  inline void trace(Args &&...args) const noexcept {
    spdlog::trace(std::forward<Args>(args)...);
  }

  /**
   * The remainder of the functions here serve to manipulate the state of the underlying
   * logging library.
   */
  void set_level(LogLevel level);
  void set_pattern(const std::string &pattern);

}; /* class Logger */

} /* namespace omulator */

