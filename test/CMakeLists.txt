# Download GoogleTest
include(FetchContent)

# Use to add tests
# From https://cliutils.gitlab.io/modern-cmake/chapters/testing/googletest.html
macro(add_unit_test TESTNAME)
  add_executable(${TESTNAME})

  target_include_directories(
    ${TESTNAME}
    PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/test/include>
      $<INSTALL_INTERFACE:include>
      $<INSTALL_INTERFACE:test/include>
  )

  target_sources_local(
    ${TESTNAME}
    PRIVATE
      ${ARGN}
  )

  configure_target(${TESTNAME} TRUE)
  target_link_libraries(
    ${TESTNAME}
    gtest
    gmock
    gtest_main
  )

  add_test(NAME ${TESTNAME} COMMAND ${TESTNAME})
  set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
endmacro()

add_subdirectory(unit)
