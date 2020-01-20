#pragma once

/**
 * Keeps #include's and #define's for intrinsic headers in one place,
 * across architectures and compilers.
 */

#ifdef OML_ARCH_X64

// MSVC family needs intrin.h on x64
#if defined(OML_COMPILER_MSVC) || defined(OML_COMPILER_CLANG_CL)
#include <intrin.h>
#endif /* if defined(OML_COMPILER_MSVC) || defined(OML_COMPILER_CLANG_CL) */
#include <immintrin.h>
#define OML_INTRIN_PAUSE() _mm_pause()
#else
#error Need to define intrinsics for target platform
#endif /* ifdef OML_ARCH_X64 */
