#pragma once

namespace omulator {
constinit const inline bool OML_DEBUGBUILD =
#ifdef NDEBUG
  false;
#else
  true;
#endif
}  // namespace omulator

#if defined(OML_COMPILER_GCC) || defined(OML_COMPILER_CLANG) || defined(OML_COMPILER_CLANG_CL)
#define OML_FORCEINLINE inline __attribute__((always_inline))
#define OML_RESTRICT    __restrict__
#define OML_PUREFUNC    __attribute__((pure))
#define OML_CONSTFUNC   __attribute__((const))

#elif defined(OML_COMPILER_MSVC)
#define OML_FORCEINLINE __forceinline
#define OML_RESTRICT    __restrict
#define OML_PUREFUNC
#define OML_CONSTFUNC
#else
#error Unknown compiler...
#endif
