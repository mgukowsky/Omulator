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

  target_include_directories(
    ${TESTNAME}
    SYSTEM
    PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/concurrentqueue>
      $<INSTALL_INTERFACE:third_party/concurrentqueue>
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
    # Prevent slower CI systems from timing out.
    # See https://stackoverflow.com/questions/68706014/gtest-discover-tests-failing-in-github-actions-process-terminated-due-to-timeou
    DISCOVERY_TIMEOUT
      60
  )
endfunction()

# Same as add_unit_test, except automatically adds src/SRCDIR/TESTNAME.cpp as a target source
function(add_unit_test_with_source TESTNAME SRCDIR)
  add_unit_test(${TESTNAME} ${PROJECT_SOURCE_DIR}/src/${SRCDIR}/${TESTNAME}.cpp ${ARGN})
endfunction()

add_unit_test(smoke)

# Disabled because this would need to link w/ pybind11, and IDGAF if this works since it's really not complicated
# add_unit_test_with_source(exception_handler util)
add_unit_test(PropertyMap)
add_unit_test(Spinlock)
add_unit_test(TypeHash)
add_unit_test(TypeString)
add_unit_test(TypeMap)
add_unit_test_with_source(Injector di)
add_unit_test_with_source(MessageQueue msg)
add_unit_test_with_source(MessageQueueFactory msg ${PROJECT_SOURCE_DIR}/src/msg/MessageQueue.cpp)
add_unit_test_with_source(MailboxEndpoint msg
  ${PROJECT_SOURCE_DIR}/src/msg/MessageQueue.cpp
  ${PROJECT_SOURCE_DIR}/src/msg/MessageQueueFactory.cpp
  ${PROJECT_SOURCE_DIR}/src/msg/MailboxEndpoint.cpp
)
add_unit_test_with_source(MailboxRouter msg
  ${PROJECT_SOURCE_DIR}/src/msg/MessageQueue.cpp
  ${PROJECT_SOURCE_DIR}/src/msg/MessageQueueFactory.cpp
  ${PROJECT_SOURCE_DIR}/src/msg/MailboxEndpoint.cpp
  ${PROJECT_SOURCE_DIR}/src/msg/MailboxSender.cpp
  ${PROJECT_SOURCE_DIR}/src/msg/MailboxReceiver.cpp
)

# TODO: adding '.' to signify the lack of a subdirectory here works, but isn't super tidy...
add_unit_test_with_source(System .
  ${PROJECT_SOURCE_DIR}/src/Component.cpp
  ${PROJECT_SOURCE_DIR}/src/di/Injector.cpp
  ${PROJECT_SOURCE_DIR}/src/Subsystem.cpp
  ${PROJECT_SOURCE_DIR}/src/msg/MessageQueue.cpp
  ${PROJECT_SOURCE_DIR}/src/msg/MessageQueueFactory.cpp
  ${PROJECT_SOURCE_DIR}/src/msg/MailboxEndpoint.cpp
  ${PROJECT_SOURCE_DIR}/src/msg/MailboxRouter.cpp
  ${PROJECT_SOURCE_DIR}/src/msg/MailboxSender.cpp
  ${PROJECT_SOURCE_DIR}/src/msg/MailboxReceiver.cpp
)

add_unit_test_with_source(Subsystem .
  ${PROJECT_SOURCE_DIR}/src/Subsystem.cpp
  ${PROJECT_SOURCE_DIR}/src/msg/MessageQueue.cpp
  ${PROJECT_SOURCE_DIR}/src/msg/MessageQueueFactory.cpp
  ${PROJECT_SOURCE_DIR}/src/msg/MailboxEndpoint.cpp
  ${PROJECT_SOURCE_DIR}/src/msg/MailboxRouter.cpp
  ${PROJECT_SOURCE_DIR}/src/msg/MailboxSender.cpp
  ${PROJECT_SOURCE_DIR}/src/msg/MailboxReceiver.cpp
)
