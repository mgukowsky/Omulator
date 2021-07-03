#pragma once

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

  ILogger(const ILogger &) = delete;
  ILogger &operator=(const ILogger &) = delete;
  ILogger(ILogger &&)                 = default;
  ILogger &operator=(ILogger &&) = default;

  virtual void critical(const char * const) = 0;
  virtual void error(const char * const)    = 0;
  virtual void warn(const char * const)     = 0;
  virtual void info(const char * const)     = 0;
  virtual void debug(const char * const)    = 0;
  virtual void trace(const char * const)    = 0;

  /**
   * The remainder of the functions here serve to manipulate the state of the underlying
   * logging library.
   */
  virtual void set_level(LogLevel level) = 0;

}; /* class Logger */

} /* namespace omulator */
