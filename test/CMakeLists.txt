include(GoogleTest)

# Use to add tests
# TESTNAME must have a corresponding ${TESTNAME}_test.cpp, in addition to any other
# source files.
# From https://cliutils.gitlab.io/modern-cmake/chapters/testing/googletest.html
function(add_unit_test TESTNAME)
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

  gtest_discover_tests(
    ${TESTNAME}
    DISCOVERY_MODE
      PRE_TEST
  )
endfunction()

# Same as add_unit_test, except automatically adds src/SRCDIR/TESTNAME.cpp as a target source
function(add_unit_test_with_source TESTNAME SRCDIR)
  add_unit_test(${TESTNAME} ${PROJECT_SOURCE_DIR}/src/${SRCDIR}/${TESTNAME}.cpp ${ARGN})
endfunction()

add_unit_test(smoke)

add_unit_test_with_source(exception_handler util)
add_unit_test(ObjectPool)
add_unit_test(ResourceWrapper)
add_unit_test(Latch ${PROJECT_SOURCE_DIR}/src/Latch.cpp)
add_unit_test(Spinlock)
add_unit_test(TypeHash)
add_unit_test(TypeString)
add_unit_test(TypeMap)
add_unit_test_with_source(Injector di)
add_unit_test_with_source(MessageBuffer msg)
add_unit_test_with_source(Package msg ${PROJECT_SOURCE_DIR}/src/msg/MessageBuffer.cpp)

add_unit_test(Worker
  ${PROJECT_SOURCE_DIR}/src/scheduler/Worker.cpp
  ${PROJECT_SOURCE_DIR}/src/Latch.cpp
)

add_unit_test(WorkerPool
  ${PROJECT_SOURCE_DIR}/src/scheduler/WorkerPool.cpp
  ${PROJECT_SOURCE_DIR}/src/scheduler/Worker.cpp
  ${PROJECT_SOURCE_DIR}/src/Latch.cpp
)

