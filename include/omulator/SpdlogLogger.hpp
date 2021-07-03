#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/util/Pimpl.hpp"

namespace omulator {
class SpdlogLogger : public ILogger {
public:
  SpdlogLogger(const LogLevel initialLevel = LogLevel::WARN);
  ~SpdlogLogger();

  void critical(const char * const msg) override;
  void error(const char * const msg) override;
  void warn(const char * const msg) override;
  void info(const char * const msg) override;
  void debug(const char * const msg) override;
  void trace(const char * const msg) override;

  void set_level(LogLevel level) override;

private:
  // spdlog.h seems to take a while to compile; using pimpl here helps to address that.
  struct Impl_;
  util::Pimpl<Impl_> impl_;
};
}  // namespace omulator
