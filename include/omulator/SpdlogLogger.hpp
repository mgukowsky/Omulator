#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/util/Pimpl.hpp"

namespace omulator {
class SpdlogLogger : public ILogger {
public:
  explicit SpdlogLogger(const LogLevel initialLevel = LogLevel::INFO);
  ~SpdlogLogger() override;

  SpdlogLogger(const SpdlogLogger &) = delete;
  SpdlogLogger &operator=(const SpdlogLogger &) = delete;
  SpdlogLogger(SpdlogLogger &&)                 = default;
  SpdlogLogger &operator=(SpdlogLogger &&) = default;

  void critical(const char * const msg, const util::SourceLocation location) override;
  void error(const char * const msg, const util::SourceLocation location) override;
  void warn(const char * const msg, const util::SourceLocation location) override;
  void info(const char * const msg, const util::SourceLocation location) override;
  void debug(const char * const msg, const util::SourceLocation location) override;
  void trace(const char * const msg, const util::SourceLocation location) override;

  void set_level(LogLevel level) override;

private:
  // spdlog.h seems to take a while to compile; using pimpl here helps to address that.
  struct Impl_;
  util::Pimpl<Impl_> impl_;
};
}  // namespace omulator
