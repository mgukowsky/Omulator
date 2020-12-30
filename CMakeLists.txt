cmake_minimum_required(
  VERSION
    3.16
)

# CMake policy settings

# Add appropriate PIE flags when POSTION_INDEPENDENT_CODE is set
cmake_policy(SET CMP0083 NEW)
# Use the MSVC_RUNTIME_LIBRARY to select and MSVC runtime
cmake_policy(SET CMP0091 NEW)
# Don't place MSVC warning flags in the default CMake cache entries
cmake_policy(SET CMP0092 NEW)

# Set the default build type. From https://blog.kitware.com/cmake-and-the-default-build-type
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "CMAKE_BUILD_TYPE not specified; defaulting to 'Debug'")
  set(
    CMAKE_BUILD_TYPE
      "Debug"
    CACHE
    STRING
    "The CMake build type. Can be one of 'Debug|Release|MinSizeRel|RelWithDebInfo'"
    FORCE
  )

  set_property(
    CACHE
    CMAKE_BUILD_TYPE
    PROPERTY
    STRINGS
      "Debug" "Release" "MinSizeRel" "RelWithDebInfo"
  )
endif()

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

message(
  STATUS
    "Starting CMake with "
    ${CMAKE_CXX_COMPILER_ID}
    " on "
    ${CMAKE_SYSTEM_NAME}
)

include(OmulatorUtils)

# Main executable
add_executable(
  ${PROJECT_NAME}
)

find_package(
  Threads
    REQUIRED
)
target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
    Threads::Threads
)

if(NOT MSVC)
  message(
    STATUS
    "Searching for SDL2; please ensure any relevant packages are installed (i.e. libsdl2-dev)"
  )

  # N.B. this uses our <project_root>/cmake/FindSDL2.cmake
  find_package(
    SDL2
      REQUIRED
  )

  # CMake prefers we call target_link_libraries in the same CMake directory
  # where the target is declared...
  target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
      ${SDL2_LIBRARY}
  )
endif()

add_subdirectory(platform)
add_subdirectory(src)

# Tests
if(OMULATOR_BUILD_TESTS)
  include(CTest)
  enable_testing()
  
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  add_subdirectory("third_party/googletest")
  add_subdirectory("test")
endif()
