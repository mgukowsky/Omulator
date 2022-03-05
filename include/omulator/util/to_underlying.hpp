#pragma once

namespace omulator::util {

/**
 * Convert an enumeration value to its underlying integral type.
 *
 * N.B. this will get added in C++23... use this as a shim till then.
 *
 * From https://stackoverflow.com/a/14589519
 */
template<typename T, typename Underlying_t = std::underlying_type_t<T>>
constexpr Underlying_t to_underlying(T val) noexcept {
  return static_cast<Underlying_t>(val);
}

}  // namespace omulator::util
