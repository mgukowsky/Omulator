#pragma once

#include <spdlog/spdlog.h>

#include <utility>

namespace omulator::logger {

/**
 * Logging functions to abstract away the underlying log library. All logging
 * functions accept a libfmt-style series of arguments, ie. log("fmt string", args...)
 */

//TODO: any way to make these noexcept?
template<typename ...Args>
inline void critical(Args &&...args) {
  spdlog::critical(std::forward<Args>(args)...);
}

template<typename ...Args>
inline void error(Args &&...args) {
  spdlog::error(std::forward<Args>(args)...);
}

template<typename ...Args>
inline void warn(Args &&...args) {
  spdlog::warn(std::forward<Args>(args)...);
}

template<typename ...Args>
inline void info(Args &&...args) {
  spdlog::info(std::forward<Args>(args)...);
}

template<typename ...Args>
inline void debug(Args &&...args) {
  spdlog::debug(std::forward<Args>(args)...);
}

template<typename ...Args>
inline void trace(Args &&...args) {
  spdlog::trace(std::forward<Args>(args)...);
}

/**
 * The remainder of the functions here serve to manipulate the state of the underlying
 * logging library.
 */
enum class LogLevel {
  OFF,
  CRITICAL,
  ERR,
  WARN,
  INFO,
  DEBUG,
  TRACE
};

void set_level(LogLevel level);

} /* namespace omulator::logger */

