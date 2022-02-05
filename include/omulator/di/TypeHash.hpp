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
  constinit const std::uint32_t FNV_BASIS_32 = 0x811C'9DC5;
  constinit const std::uint32_t FNV_PRIME_32 = 0x0100'0193;
  constinit const std::uint64_t FNV_BASIS_64 = 0xCBF2'9CE4'8422'2325;
  constinit const std::uint64_t FNV_PRIME_64 = 0x0000'0100'0000'01B3;

  template<typename HashSize_t, HashSize_t fnv_basis, HashSize_t fnv_prime>
  consteval HashSize_t fnv1a_hash(const std::string_view sv) {
    HashSize_t hash = fnv_basis;

    for(const auto c : sv) {
      hash = (hash ^ static_cast<HashSize_t>(c)) * fnv_prime;
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

/**
 * Note that TypeHash is a templated inline variable, hence the need for decltype (and the
 * remove_const_t) here.
 */
using Hash_t = std::remove_const_t<decltype(TypeHash<void>)>;

} /* namespace omulator::di */
