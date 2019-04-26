#include "omulator/logger.hpp"

void omulator::logger::set_level(omulator::logger::LogLevel level) {
  switch(level) {
    case omulator::logger::LogLevel::OFF:
      spdlog::set_level(spdlog::level::off);
      break;

    case omulator::logger::LogLevel::CRITICAL:
      spdlog::set_level(spdlog::level::critical);
      break;

    case omulator::logger::LogLevel::ERROR:
      spdlog::set_level(spdlog::level::err);
      break;

    case omulator::logger::LogLevel::WARN:
      spdlog::set_level(spdlog::level::warn);
      break;

    case omulator::logger::LogLevel::INFO:
      spdlog::set_level(spdlog::level::info);
      break;
  
    case omulator::logger::LogLevel::DEBUG:
      spdlog::set_level(spdlog::level::debug);
      break;

    case omulator::logger::LogLevel::TRACE:
      spdlog::set_level(spdlog::level::trace);
      break;

    default:
      break;
  }
}

