#pragma once

#if defined(OML_COMPILER_CLANG) || defined(OML_COMPILER_CLANG_CL)
/**
 * As of 12/21 Clang doesn't have support for std::source_location, so we create a shim for it here.
 * Unfortunately this will cause some whitespace to be added to SpdlogLogger's log messages since it
 * will have nothing to fill the fields in its format string for file name, line number, and
 * function name, but hopefully clang will add support for this feature in the not too distant
 * future and render this a moot point.
 */
namespace omulator::util {

class SourceLocation {
public:
  const char                 *file_name() const noexcept { return ""; }
  const char                 *line() const noexcept { return ""; }
  const char                 *function_name() const noexcept { return ""; }
  static const SourceLocation current() noexcept { return {}; }
};

}  // namespace omulator::util
#else
#include <source_location>
namespace omulator::util {
using SourceLocation = std::source_location;
}  // namespace omulator::util
#endif
