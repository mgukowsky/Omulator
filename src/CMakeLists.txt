configure_target(${PROJECT_NAME} FALSE)

# If MSVC, specify we don't want to display a console window
if(CMAKE_BUILD_TYPE MATCHES Release AND MSVC)
  set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
      WIN32_EXECUTABLE
        ON
  )
endif()

# Use IPO if Release and available
if(CMAKE_BUILD_TYPE MATCHES Release AND NOT (${CMAKE_CXX_COMPILER_ID} MATCHES "MSVC"))
  check_ipo_supported(RESULT is_ipo_supported)
  if(is_ipo_supported)
    set_target_properties(
      ${PROJECT_NAME}
        PROPERTIES
          INTERPROCEDURAL_OPTIMIZATION
            TRUE
    )
  endif()
endif()

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/spdlog/include>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:third_party/spdlog/include>
)

target_sources(
  ${PROJECT_NAME}
  PRIVATE
    Latch.cpp
    Logger.cpp
    main.cpp
)

# Finally, require a pass through clang-tidy
# Unfortunately, clang-tidy barfs on the -std:c++17 flag with MSVC :(
# And it barfs on non-clang flags in GCC :( :(
if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" AND ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  add_clang_tidy_pass(${PROJECT_NAME})
endif()
