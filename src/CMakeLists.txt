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
)

target_sources(
  ${PROJECT_NAME}
  PRIVATE
    Latch.cpp
    Logger.cpp
    main.cpp
)

add_subdirectory(scheduler)
