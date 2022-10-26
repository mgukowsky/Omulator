# Instructions to compile imgui as a static library

set(IMGUI_LIB_NAME imgui)
add_library(${IMGUI_LIB_NAME})

set_target_properties(
  ${IMGUI_LIB_NAME}
  PROPERTIES
    CXX_STANDARD
      20
    CXX_STANDARD_REQUIRED
      ON
    CXX_EXTENSIONS
      OFF
    MSVC_RUNTIME_LIBRARY
      MultiThreaded$<$<CONFIG:Debug>:Debug>DLL
)

target_compile_definitions(
  ${IMGUI_LIB_NAME}
  PUBLIC
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},Release>:NDEBUG>

    # _REENTRANT signals parts of the stdlib to use threadsafe functions
    $<$<BOOL:UNIX>:_REENTRANT>
    )

target_include_directories(
  ${IMGUI_LIB_NAME}
  PUBLIC
    ${PROJECT_SOURCE_DIR}/third_party/imgui
)

target_sources(
  ${IMGUI_LIB_NAME}
  PRIVATE
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_vulkan.cpp
)

if(MSVC)
  target_compile_definitions(
    ${IMGUI_LIB_NAME}
    PUBLIC
      _MBCS
      _WINDOWS
      WIN32
      # Magic macro to limit how much of the Windows header gets pulled in
      WIN32_LEAN_AND_MEAN
  )

  target_compile_options(
    ${IMGUI_LIB_NAME}
    PRIVATE
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:/Zi /Ob0 /Od /GS /sdl /RTC1>
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Release>:/O2 /Ob2 /Gw /GS- /Gy>
      /arch:AVX2
      /GR
      /EHsc
      /utf-8
  )

  target_sources(
    ${IMGUI_LIB_NAME}
    PRIVATE
      third_party/imgui/backends/imgui_impl_win32.cpp
  )
else()
  target_include_directories(
    ${IMGUI_LIB_NAME}
    SYSTEM
    PUBLIC
      # Set by find_package(SDL2)
      ${SDL2_INCLUDE_DIR}
  )

  target_compile_options(
    ${IMGUI_LIB_NAME}
    PRIVATE
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:-g -O0>
      $<$<STREQUAL:${CMAKE_BUILD_TYPE},Release>:-O3>
  )

  target_sources(
    ${IMGUI_LIB_NAME}
    PRIVATE
      third_party/imgui/backends/imgui_impl_sdl.cpp
  )
endif()
