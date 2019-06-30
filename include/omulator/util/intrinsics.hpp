#pragma once

/**
 * Keeps #include's for intrinsic headers in one place, across architectures
 * and compilers
 */

#if defined(OML_COMPILER_MSVC) || defined(OML_COMPILER_CLANG_CL)
#if defined(OML_ARCH_X64)
#include <intrin.h>
#elif defined(OML_ARCH_ARM)
#error Need to define MSVC intrin header for ARM
#endif
#elif defined(OML_COMPILER_GCC) || defined(OML_COMPILER_CLANG)
/* no header needed */
#endif
