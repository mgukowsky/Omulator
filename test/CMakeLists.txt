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
      unit/${TESTNAME}_test.cpp
      ${ARGN}
  )

  configure_target(${TESTNAME})
  target_link_libraries(
    ${TESTNAME}
    gtest
    gmock
    gtest_main
  )

  add_test(NAME ${TESTNAME} COMMAND ${TESTNAME})
  set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
endmacro()

add_unit_test(smoke)

add_unit_test(exception_handler)
add_unit_test(ObjectPool)
add_unit_test(ResourceWrapper)
add_unit_test(Latch ${PROJECT_SOURCE_DIR}/src/Latch.cpp)
add_unit_test(Spinlock)

add_unit_test(Worker
  ${PROJECT_SOURCE_DIR}/src/scheduler/Worker.cpp
  ${PROJECT_SOURCE_DIR}/src/Latch.cpp
)

add_unit_test(WorkerPool
  ${PROJECT_SOURCE_DIR}/src/scheduler/WorkerPool.cpp
  ${PROJECT_SOURCE_DIR}/src/scheduler/Worker.cpp
  ${PROJECT_SOURCE_DIR}/src/Latch.cpp
)

