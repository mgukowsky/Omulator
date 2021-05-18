#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>

/**
 * Utility to create a unique hash for individual types at compile time.
 *
 * Based on https://github.com/Manu343726/ctti/blob/master/include/ctti/detail/hash.hpp
 */
namespace omulator::di {

namespace detail {
  constinit const std::uint32_t FNV_BASIS_32 = 0x811c9dc5ULL;
  constinit const std::uint32_t FNV_PRIME_32 = 0x01000193ULL;
  constinit const std::uint64_t FNV_BASIS_64 = 0xcbf29ce484222325ULL;
  constinit const std::uint64_t FNV_PRIME_64 = 0x00000100000001B3ULL;

  template<typename Hash_t, Hash_t fnv_basis, Hash_t fnv_prime>
  consteval Hash_t fnv1a_hash(const std::string_view sv) {
    Hash_t hash = fnv_basis;

    for(const auto c : sv) {
      hash = (hash ^ static_cast<Hash_t>(c)) * fnv_prime;
    }
    return hash;
  }

  /**
   * Helper function to generate a unique string depending on the type argument. This works because
   * the magic macros used in this function will return the full name of the function _including_
   * the type argument.
   */
  template<typename T>
  consteval std::string_view uid_helper() {
#if defined(OML_COMPILER_MSVC) || defined(OML_COMPILER_CLANG_CL)
    return __FUNCSIG__;
#elif defined(OML_COMPILER_GCC) || defined(OML_COMPILER_CLANG)
    return __PRETTY_FUNCTION__;
#else
#error "This compiler cannot support the compile-time UID feature"
#endif
  }
} /* namespace detail */

// N.B. that both the 32 and 64 bit versions will decay the type
template<typename T>
constinit const inline auto TypeHash32 =
  detail::fnv1a_hash<std::uint32_t, detail::FNV_BASIS_32, detail::FNV_PRIME_32>(
    detail::uid_helper<std::decay_t<T>>());

template<typename T>
constinit const inline auto TypeHash64 =
  detail::fnv1a_hash<std::uint64_t, detail::FNV_BASIS_64, detail::FNV_PRIME_64>(
    detail::uid_helper<std::decay_t<T>>());

// Use the 64 bit type hash by default
template<typename T>
constinit const inline auto TypeHash = TypeHash64<T>;

} /* namespace omulator::di */
