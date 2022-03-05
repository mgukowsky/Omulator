#pragma once

#include <chrono>
#include <cstdint>

/**
 * A set of aliases for common types.
 */
namespace omulator {

using S8  = std::int8_t;
using S16 = std::int16_t;
using S32 = std::int32_t;
using S64 = std::int64_t;

using U8  = std::uint8_t;
using U16 = std::uint16_t;
using U32 = std::uint32_t;
using U64 = std::uint64_t;

using TimePoint_t = decltype(std::chrono::steady_clock::now());
} /* namespace omulator */
