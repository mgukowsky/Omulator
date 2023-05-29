cmake_minimum_required(
  VERSION
    3.19
)

# CMake policy settings

# Add appropriate PIE flags when POSTION_INDEPENDENT_CODE is set
cmake_policy(SET CMP0083 NEW)
# Use the MSVC_RUNTIME_LIBRARY to select and MSVC runtime
cmake_policy(SET CMP0091 NEW)
# Don't place MSVC warning flags in the default CMake cache entries
cmake_policy(SET CMP0092 NEW)

list(
  APPEND
    CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_LIST_DIR}/cmake
    ${CMAKE_CURRENT_LIST_DIR}/third_party/cmake/FindSDL2
)

include(ConfigureTarget)

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

# Main executable
add_executable(
  ${PROJECT_NAME}
)

find_package(
  Threads
    REQUIRED
)

find_package(
  Vulkan
    REQUIRED
  # Commented out since older Vulkan SDKs may not include this
  # COMPONENTS
  #   glslc
)

# Compile LuaJIT using its platform-specific build system
set(LUAJIT_DIR ${PROJECT_SOURCE_DIR}/third_party/LuaJIT/src)
if(MSVC)
  set(LUAJIT_MAKE_CMD "msvcbuild.bat")
  set(LUAJIT_MAKE_CMD_ARGS "static")
  # LuaJIT on windows seemingly splits the lua bindings and LuaJIT into 2 separate libs
  set(LUA_LIB ${LUAJIT_DIR}/lua51.lib)
  set(LUAJIT_LIB ${LUAJIT_DIR}/luajit.lib)
else()
  set(LUAJIT_MAKE_CMD "make")
  set(LUAJIT_MAKE_CMD_ARGS "-j")
  set(LUA_LIB "")
  set(LUAJIT_LIB ${LUAJIT_DIR}/libluajit.a)
endif()

add_custom_command(
  COMMAND
    # TODO: this cmd may not necessarily respect the CMake toolchain file...
    ${LUAJIT_MAKE_CMD}
  ARGS
    ${LUAJIT_MAKE_CMD_ARGS}
  OUTPUT
    ${LUA_LIB}
    ${LUAJIT_LIB}
  WORKING_DIRECTORY
    ${LUAJIT_DIR}
  COMMENT
    "Compiling LuaJIT"
)

add_custom_target(
  LUAJIT_COMPILATION
  DEPENDS
    ${LUA_LIB}
    ${LUAJIT_LIB}
)

add_dependencies(
  ${PROJECT_NAME}
    LUAJIT_COMPILATION
)

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
    ${LUA_LIB}
    ${LUAJIT_LIB}
)

find_program(GLSLC_EXE NAMES glslc HINTS Vulkan::glslc)

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
      readline
  )
endif()

# SDL must have been found before this is called
include(libimgui)

add_subdirectory("third_party/docopt.cpp")
add_subdirectory("third_party/glm")
add_subdirectory("third_party/spdlog")
add_subdirectory("third_party/tinyobjloader")
add_subdirectory("third_party/vk-bootstrap")
add_subdirectory("third_party/VulkanMemoryAllocator")

# Add exception handling to some libraries which don't define it in order to make MSVC and clang-cl
# happy
if(MSVC)
  configure_target(docopt)
  configure_target(spdlog)
  configure_target(tinyobjloader)
endif()

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
    Threads::Threads
    spdlog::spdlog
    Vulkan::Vulkan
    glm::glm
    imgui
    docopt
    tinyobjloader
    vk-bootstrap::vk-bootstrap
    VulkanMemoryAllocator
    ${CMAKE_DL_LIBS}
)

configure_target(${PROJECT_NAME})

#TODO: Eventually want to turn this back on
# If MSVC, specify we don't want to display a console window
#if(CMAKE_BUILD_TYPE MATCHES Release AND MSVC)
#  set_target_properties(
#    ${PROJECT_NAME}
#    PROPERTIES
#      WIN32_EXECUTABLE
#        ON
#  )
#endif()

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# We include third-party headers as system headers so that
# they don't trigger warnings
target_include_directories(
  ${PROJECT_NAME}
  SYSTEM
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/spdlog/include>
    $<INSTALL_INTERFACE:third_party/spdlog/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/glm>
    $<INSTALL_INTERFACE:third_party/glm>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/imgui>
    $<INSTALL_INTERFACE:third_party/imgui>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/concurrentqueue>
    $<INSTALL_INTERFACE:third_party/concurrentqueue>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/docopt.cpp>
    $<INSTALL_INTERFACE:third_party/docopt.cpp>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/tinyobjloader>
    $<INSTALL_INTERFACE:third_party/tinyobjloader>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/vk-bootstrap/src>
    $<INSTALL_INTERFACE:third_party/vk-bootstrap/src>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/VulkanMemoryAllocator/include>
    $<INSTALL_INTERFACE:third_party/VulkanMemoryAllocator/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/LuaJIT/src>
    $<INSTALL_INTERFACE:third_party/LuaJIT/src>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/sol2/include>
    $<INSTALL_INTERFACE:third_party/sol2/include>
)

# Select which platform folder to use
# N.B. "sdl" assumes the compiler is gcc or clang, NOT msvc...
if(MSVC)
  set(PLATFORM_DIR "platform/win32")
else()
  set(PLATFORM_DIR "platform/sdl")

  # We include third-party headers as system headers so that
  # they don't trigger warnings
  target_include_directories(
    ${PROJECT_NAME}
    SYSTEM
    PUBLIC
      # Set by find_package(SDL2)
      ${SDL2_INCLUDE_DIR}
  )
endif()

include(SourceFileManifest)
target_sources(
  ${PROJECT_NAME}
  PRIVATE
  ${OML_SOURCE_FILE_MANIFEST}
)

# Shader compilation
# Based on https://stackoverflow.com/a/68457439
set(SHADER_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
add_custom_command(
  COMMAND
    ${CMAKE_COMMAND} -E make_directory ${SHADER_OUT_DIR}
  OUTPUT
    ${SHADER_OUT_DIR}
  COMMENT
    "Creating directory ${SHADER_OUT_DIR}"
)

foreach(SHADER IN LISTS OML_SHADER_FILE_MANIFEST)
  get_filename_component(FILENAME ${SHADER} NAME)
  set(SHADER_SRC ${PROJECT_SOURCE_DIR}/${SHADER})
  set(SHADER_BIN ${SHADER_OUT_DIR}/${FILENAME}.spv)
  add_custom_command(
    COMMAND
      ${GLSLC_EXE}
    ARGS
      -o ${SHADER_BIN}
      --target-env=vulkan1.1
      -Werror
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:-g>
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:-O0>
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Release>:-O>
      ${SHADER_SRC}
    MAIN_DEPENDENCY
      ${SHADER_SRC}
    OUTPUT
      ${SHADER_BIN}
    DEPENDS
      ${SHADER_OUT_DIR}
    COMMENT
      "Compiling shader ${SHADER}"
  )

  list(
    APPEND
    COMPILED_SHADERS
      ${SHADER_BIN}
  )
endforeach()

if(GLSLC_EXE)
  add_custom_target(
    SHADER_COMPILATION
    DEPENDS
      ${COMPILED_SHADERS}
  )

  add_dependencies(
    ${PROJECT_NAME}
      SHADER_COMPILATION
  )
else()
  # TODO: This is a stopgap until we can get glslc into our CI environment; tests may run w/o the
  # compiled shaders, but our app won't.
  message(WARNING "Skipping shader compilation because glslc could not be found")
endif()

# Download external assets
# TODO: This will download a set of assets for each build type; download once and copy/symlink to
# each folder as needed
include(DownloadedAssetManifest)
foreach(URL IN LISTS OML_DOWNLOADED_ASSET_MANIFEST)
  get_filename_component(FILENAME ${URL} NAME)
  set(OUTFILE ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME})
  if(NOT EXISTS ${OUTFILE})
    message(STATUS "Downloading ${FILENAME}")
    file(
      DOWNLOAD
        ${URL}
        ${OUTFILE}
        SHOW_PROGRESS
    )
  endif()
endforeach()

# Tests
if(OMULATOR_BUILD_TESTS)
  include(CTest)
  enable_testing()

  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  add_subdirectory("third_party/googletest")
  add_subdirectory("test")
endif()
