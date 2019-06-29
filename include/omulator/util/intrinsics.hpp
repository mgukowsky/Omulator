#pragma once

/**
 * Keeps #include's for intrinsic headers in one place, across architectures
 * and compilers
 */

#if defined(_MSC_VER)
#if defined(OML_ARCH_X64)
#include <intrin.h>
#elif defined(OML_ARCH_ARM)
#error Need to define MSVC intrin header for ARM
#endif
#elif defined(__GNUC__) || defined(__clang__)
/* no header needed */
#endif
