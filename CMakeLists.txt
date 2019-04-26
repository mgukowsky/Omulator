cmake_minimum_required(
  VERSION
    3.12
)

list(
  APPEND
    CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_LIST_DIR}/cmake
    ${CMAKE_CURRENT_LIST_DIR}/third_party/cmake/FindSDL2
)

option(
  OMULATOR_BUILD_TESTS
  "Build test executables"
  ON
)

# Override CMake default options by passing it the Overrides.cmake module.
# N.B. this is the preferred (although slightly magical) way to do this,
# according to my research
set(CMAKE_USER_MAKE_RULES_OVERRIDE Overrides)

project(
  Omulator
  VERSION 
    0.0.0
  LANGUAGES
    CXX
)

message(STATUS "Starting CMake with " ${CMAKE_CXX_COMPILER_ID} " on " ${CMAKE_SYSTEM_NAME})

# Need pthread on *nix
find_package(
  Threads
    REQUIRED
)

if(NOT MSVC)
  message(
    STATUS
      "Searching for SDL2; please ensure any relevant packages are installed (i.e. libsdl2-dev)"
  )

  # N.B. this uses our <project_root>/cmake/FindSDL2.cmake
  find_package(SDL2 REQUIRED)
endif()

include(CheckIPOSupported)
include(OmulatorUtils)

# Main executable
add_executable(
  ${PROJECT_NAME}
)
add_subdirectory(src)

# Tests
if(OMULATOR_BUILD_TESTS)
  include(CTest)
  enable_testing()

  add_subdirectory("test")
endif()
