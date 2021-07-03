#include "omulator/SpdlogLogger.hpp"

// Disable warnings from spdlog when using clang-cl
#if defined(OML_COMPILER_CLANG_CL)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#if defined(OML_COMPILER_CLANG_CL)
#pragma clang diagnostic pop
#endif

#include <memory>

namespace omulator {

struct SpdlogLogger::Impl_ {
  Impl_() : logger(spdlog::stdout_color_mt("SpdlogLogger")) { }
  ~Impl_() = default;
  std::shared_ptr<spdlog::logger> logger;
};

SpdlogLogger::SpdlogLogger(const ILogger::LogLevel initialLevel) { set_level(initialLevel); }

SpdlogLogger::~SpdlogLogger() { impl_->logger->flush(); }

void SpdlogLogger::critical(const char * const msg) { impl_->logger->critical(msg); }
void SpdlogLogger::error(const char * const msg) { impl_->logger->error(msg); }
void SpdlogLogger::warn(const char * const msg) { impl_->logger->warn(msg); }
void SpdlogLogger::info(const char * const msg) { impl_->logger->info(msg); }
void SpdlogLogger::debug(const char * const msg) { impl_->logger->debug(msg); }
void SpdlogLogger::trace(const char * const msg) { impl_->logger->trace(msg); }

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
