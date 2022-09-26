#pragma once

#include "omulator/util/SourceLocation.hpp"

#include <sstream>
#include <string>
#include <string_view>

namespace omulator {

/**
 * Logging class to abstract away the underlying log library. All logging
 * functions accept a libfmt-style series of arguments, ie. log("fmt string", args...)
 */

class ILogger {
public:
  enum class LogLevel {
    OFF,
    CRITICAL,
    ERR,
    WARN,
    INFO,
    DEBUG,
    TRACE,
  };
  ILogger()          = default;
  virtual ~ILogger() = default;

  ILogger(const ILogger &)            = delete;
  ILogger &operator=(const ILogger &) = delete;
  ILogger(ILogger &&)                 = default;
  ILogger &operator=(ILogger &&)      = default;

  virtual void critical(const char * const         msg,
                        const util::SourceLocation location = util::SourceLocation::current()) = 0;
  virtual void error(const char * const         msg,
                     const util::SourceLocation location = util::SourceLocation::current())    = 0;
  virtual void warn(const char * const         msg,
                    const util::SourceLocation location = util::SourceLocation::current())     = 0;
  virtual void info(const char * const         msg,
                    const util::SourceLocation location = util::SourceLocation::current())     = 0;
  virtual void debug(const char * const         msg,
                     const util::SourceLocation location = util::SourceLocation::current())    = 0;
  virtual void trace(const char * const         msg,
                     const util::SourceLocation location = util::SourceLocation::current())    = 0;

  /**
   * The remainder of the functions here serve to manipulate the state of the underlying
   * logging library.
   */
  virtual void set_level(LogLevel level) = 0;

  /**
   * Convenience methods; no need to override these.
   */
  void critical(const std::string         &msg,
                const util::SourceLocation location = util::SourceLocation::current()) {
    critical(msg.c_str(), location);
  }
  void error(const std::string         &msg,
             const util::SourceLocation location = util::SourceLocation::current()) {
    error(msg.c_str(), location);
  }
  void warn(const std::string         &msg,
            const util::SourceLocation location = util::SourceLocation::current()) {
    warn(msg.c_str(), location);
  }
  void info(const std::string         &msg,
            const util::SourceLocation location = util::SourceLocation::current()) {
    info(msg.c_str(), location);
  }
  void debug(const std::string         &msg,
             const util::SourceLocation location = util::SourceLocation::current()) {
    debug(msg.c_str(), location);
  }
  void trace(const std::string         &msg,
             const util::SourceLocation location = util::SourceLocation::current()) {
    trace(msg.c_str(), location);
  }

  void critical(const std::string_view     msg,
                const util::SourceLocation location = util::SourceLocation::current()) {
    critical(msg.data(), location);
  }
  void error(const std::string_view     msg,
             const util::SourceLocation location = util::SourceLocation::current()) {
    error(msg.data(), location);
  }
  void warn(const std::string_view     msg,
            const util::SourceLocation location = util::SourceLocation::current()) {
    warn(msg.data(), location);
  }
  void info(const std::string_view     msg,
            const util::SourceLocation location = util::SourceLocation::current()) {
    info(msg.data(), location);
  }
  void debug(const std::string_view     msg,
             const util::SourceLocation location = util::SourceLocation::current()) {
    debug(msg.data(), location);
  }
  void trace(const std::string_view     msg,
             const util::SourceLocation location = util::SourceLocation::current()) {
    trace(msg.data(), location);
  }

  void critical(const std::stringstream   &msg,
                const util::SourceLocation location = util::SourceLocation::current()) {
    critical(msg.str().c_str(), location);
  }
  void error(const std::stringstream   &msg,
             const util::SourceLocation location = util::SourceLocation::current()) {
    error(msg.str().c_str(), location);
  }
  void warn(const std::stringstream   &msg,
            const util::SourceLocation location = util::SourceLocation::current()) {
    warn(msg.str().c_str(), location);
  }
  void info(const std::stringstream   &msg,
            const util::SourceLocation location = util::SourceLocation::current()) {
    info(msg.str().c_str(), location);
  }
  void debug(const std::stringstream   &msg,
             const util::SourceLocation location = util::SourceLocation::current()) {
    debug(msg.str().c_str(), location);
  }
  void trace(const std::stringstream   &msg,
             const util::SourceLocation location = util::SourceLocation::current()) {
    trace(msg.str().c_str(), location);
  }

}; /* class Logger */

} /* namespace omulator */
