#include "omulator/Logger.hpp"

omulator::Logger::Logger(const omulator::Logger::LogLevel initialLevel,
                         const std::string &pattern) {
  set_level(initialLevel);
  spdlog::set_pattern(pattern);
  info("Initializing logger!");
}

omulator::Logger::~Logger() { info("Shutting down logger!"); }

void omulator::Logger::set_level(omulator::Logger::LogLevel level) {
  switch(level) {
    case LogLevel::OFF:
      spdlog::set_level(spdlog::level::off);
      break;

    case LogLevel::CRITICAL:
      spdlog::set_level(spdlog::level::critical);
      break;

    case LogLevel::ERR:
      spdlog::set_level(spdlog::level::err);
      break;

    case LogLevel::WARN:
      spdlog::set_level(spdlog::level::warn);
      break;

    case LogLevel::INFO:
      spdlog::set_level(spdlog::level::info);
      break;

    case LogLevel::DEBUG:
      spdlog::set_level(spdlog::level::debug);
      break;

    case LogLevel::TRACE:
      spdlog::set_level(spdlog::level::trace);
      break;

    default:
      break;
  }
}

void omulator::Logger::set_pattern(const std::string &pattern) { spdlog::set_pattern(pattern); }
