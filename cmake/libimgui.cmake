# Instructions to compile imgui as a static library
include(ConfigureTarget)

set(IMGUI_LIB_NAME imgui)
add_library(${IMGUI_LIB_NAME})

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
target_link_libraries(
  ${IMGUI_LIB_NAME}
  PUBLIC
    Vulkan::Vulkan
)

if(MSVC)
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

  target_sources(
    ${IMGUI_LIB_NAME}
    PRIVATE
      third_party/imgui/backends/imgui_impl_sdl.cpp
  )
endif()

configure_target(${IMGUI_LIB_NAME} NOWARNINGS)

