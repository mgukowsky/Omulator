#include <iostream>
#include <string_view>
#include <type_traits>
#include <vector>

namespace omulator::util {

namespace detail {
  consteval std::string_view typeStringCommon(std::string_view  rawString,
                                              const std::size_t startIdx,
                                              const std::size_t endIdx) {
    return rawString.substr(startIdx, endIdx);
  }

  /**
   * GCC and clang both place the typename of the type given as T in metastring two characters after
   * and '=' sign (e.g. "... T = "), with a small difference in the first character that comes after
   * the type name.
   */
  consteval std::string_view gccTypeString(std::string_view rawString,
                                           const char       terminalChar = ';') {
    const std::size_t startChar = rawString.find_first_of('=') + 2;
    const std::size_t endChar   = rawString.find_first_of(terminalChar);

    return typeStringCommon(rawString, startChar, endChar - startChar);
  }

  /**
   * MSVC's mangling is slightly more verbose. The type name will be prefixed by 'metastring',
   * followed by a '<' character, and will end will a '>)' sequence, which accounts for the +/- 1
   * offsets in the two indices.
   */
  consteval std::string_view msvcTypeString(std::string_view rawString) {
    const std::string_view startString = "metastring";
    const std::size_t      startIdx    = rawString.find(startString) + startString.size() + 1;
    const std::size_t      endIdx      = rawString.find_first_of('(') - 1;

    return rawString.substr(startIdx, endIdx - startIdx);
  }

  /**
   * Most compilers have a magical macro that can return the name of the enclosing function,
   * including template info.
   */
  template<typename T>
  consteval std::string_view metastring() {
#if defined(OML_COMPILER_MSVC) || defined(OML_COMPILER_CLANG_CL)
    return msvcTypeString(__FUNCSIG__);
#elif defined(OML_COMPILER_CLANG)
    return gccTypeString(std::string_view(__PRETTY_FUNCTION__), ']');
#elif defined(OML_COMPILER_GCC)
    return gccTypeString(std::string_view(__PRETTY_FUNCTION__));
#else
#error "This compiler cannot support the compile-time type string feature."
#endif
  }
}  // namespace detail

/**
 * Return the name of a given type as a string_view, computed at compile time.
 */
template<typename Raw_t, typename T = std::remove_pointer_t<std::decay_t<Raw_t>>>
constinit const inline std::string_view TypeString = detail::metastring<T>();

}  // namespace omulator::util
