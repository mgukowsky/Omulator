#include "omulator/main.hpp"

#include <Windows.h>
#include <cstdlib>

int WINAPI WinMain([[maybe_unused]] HINSTANCE hInstance,
                   [[maybe_unused]] HINSTANCE hPrevInstance,
                   [[maybe_unused]] LPSTR lpCmdLine,
                   [[maybe_unused]] int nShowCmd) {
  /**
   * __argc and __argv are magic macros populated by the runtime.
   * Since __argv is a char**, we have a rare valid use of a const_cast :)
   */
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return ::main(__argc, const_cast<const char **>(__argv));
}