#pragma once

// TODO: In C++20 the likely and unlikely macros should be replaced with
// [[likely]] and [[unlikely]]
#if defined(OML_COMPILER_GCC) || defined(OML_COMPILER_CLANG) || defined(OML_COMPILER_CLANG_CL)
#define OML_LIKELY(x) __builtin_expect(!!(x), 1)
#define OML_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define OML_FORCEINLINE inline __attribute__((always_inline))
#elif defined(OML_COMPILER_MSVC)

// N.B. MSVC has no branch prediction keywords
#define OML_LIKELY(x) x
#define OML_UNLIKELY(x) x
#define OML_FORCEINLINE __forceinline
#else
#error Unknown compiler...
#endif
