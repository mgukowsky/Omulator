#pragma once

#include "omulator/oml_defines.hpp"

namespace omulator::util {
/**
 * Given that we're inherently dealing with raw-memory manipulation throughout this project, this
 * function gives us some syntactic sugar over the common reinterpret_cast pattern.
 */
template<typename To_t, typename From_t>
OML_FORCEINLINE To_t &reinterpret(From_t *t) noexcept {
  return *(reinterpret_cast<To_t *>(t));
}
}  // namespace omulator::util
