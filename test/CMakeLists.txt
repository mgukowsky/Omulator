# Use to add tests
# TESTNAME must have a corresponding ${TESTNAME}_test.cpp, in addition to any other
# source files.
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

  target_sources(
    ${TESTNAME}
    PRIVATE
      ${TESTNAME}_test.cpp
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
