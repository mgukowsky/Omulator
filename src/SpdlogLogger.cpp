#include "omulator/SpdlogLogger.hpp"

// Disable warnings from spdlog when using clang-cl
#if defined(OML_COMPILER_CLANG_CL)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include <spdlog/fmt/bundled/color.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#if defined(OML_COMPILER_CLANG_CL)
#pragma clang diagnostic pop
#endif

#include <memory>

using fmt::terminal_color;

namespace {
constexpr const char * const CONCISE_FMTSTR = "{}";
constexpr const char * const VERBOSE_FMTSTR = "{}:{} ({}): {}";

auto colorize_string(const terminal_color color, const std::string &msg) {
  return fmt::format(fmt::fg(color), msg);
}

}  // namespace

namespace omulator {

struct SpdlogLogger::Impl_ {
  Impl_() : logger(spdlog::stdout_color_mt("SpdlogLogger")) { }

  ~Impl_() = default;
  std::shared_ptr<spdlog::logger> logger;
};

SpdlogLogger::SpdlogLogger(const ILogger::LogLevel initialLevel, const ILogger::Verbosity verbosity)
  : verbosity_(verbosity) {
  impl_->logger->set_pattern("%D:%H:%M:%S:%f [TID: %t][%l]: %v");
  set_level(initialLevel);
}

SpdlogLogger::~SpdlogLogger() { impl_->logger->flush(); }

void SpdlogLogger::critical(const char * const msg, const util::SourceLocation location) {
  if(verbosity_ == Verbosity::VERBOSE) {
    impl_->logger->critical(
      VERBOSE_FMTSTR, location.file_name(), location.line(), location.function_name(), msg);
  }
  else {
    impl_->logger->critical(CONCISE_FMTSTR, colorize_string(terminal_color::bright_magenta, msg));
  }
}

void SpdlogLogger::error(const char * const msg, const util::SourceLocation location) {
  if(verbosity_ == Verbosity::VERBOSE) {
    impl_->logger->error(
      VERBOSE_FMTSTR, location.file_name(), location.line(), location.function_name(), msg);
  }
  else {
    impl_->logger->error(CONCISE_FMTSTR, colorize_string(terminal_color::bright_red, msg));
  }
}

void SpdlogLogger::warn(const char * const msg, const util::SourceLocation location) {
  if(verbosity_ == Verbosity::VERBOSE) {
    impl_->logger->warn(
      VERBOSE_FMTSTR, location.file_name(), location.line(), location.function_name(), msg);
  }
  else {
    impl_->logger->warn(CONCISE_FMTSTR, colorize_string(terminal_color::bright_yellow, msg));
  }
}

void SpdlogLogger::info(const char * const msg, const util::SourceLocation location) {
  if(verbosity_ == Verbosity::VERBOSE) {
    impl_->logger->info(
      VERBOSE_FMTSTR, location.file_name(), location.line(), location.function_name(), msg);
  }
  else {
    impl_->logger->info(CONCISE_FMTSTR, colorize_string(terminal_color::cyan, msg));
  }
}

void SpdlogLogger::debug(const char * const msg, const util::SourceLocation location) {
  if(verbosity_ == Verbosity::VERBOSE) {
    impl_->logger->debug(
      VERBOSE_FMTSTR, location.file_name(), location.line(), location.function_name(), msg);
  }
  else {
    impl_->logger->debug(CONCISE_FMTSTR, colorize_string(terminal_color::bright_white, msg));
  }
}

void SpdlogLogger::trace(const char * const msg, const util::SourceLocation location) {
  if(verbosity_ == Verbosity::VERBOSE) {
    impl_->logger->trace(
      VERBOSE_FMTSTR, location.file_name(), location.line(), location.function_name(), msg);
  }
  else {
    impl_->logger->trace(CONCISE_FMTSTR, colorize_string(terminal_color::white, msg));
  }
}

void SpdlogLogger::set_level(ILogger::LogLevel level) {
  switch(level) {
    case ILogger::LogLevel::OFF:
      impl_->logger->set_level(spdlog::level::off);
      break;

    case ILogger::LogLevel::CRITICAL:
      impl_->logger->set_level(spdlog::level::critical);
      break;

    case ILogger::LogLevel::ERR:
      impl_->logger->set_level(spdlog::level::err);
      break;

    case ILogger::LogLevel::WARN:
      impl_->logger->set_level(spdlog::level::warn);
      break;

    case ILogger::LogLevel::INFO:
      impl_->logger->set_level(spdlog::level::info);
      break;

    case ILogger::LogLevel::DEBUG:
      impl_->logger->set_level(spdlog::level::debug);
      break;

    case ILogger::LogLevel::TRACE:
      impl_->logger->set_level(spdlog::level::trace);
      break;

    default:
      break;
  }
}

}  // namespace omulator
