set(
  OML_SOURCE_FILE_MANIFEST
    src/main.cpp
    src/Clock.cpp
    src/Component.cpp
    src/InputHandler.cpp
    src/Interpreter.cpp
    src/NullWindow.cpp
    src/SpdlogLogger.cpp
    src/Subsystem.cpp
    src/VulkanBackend.cpp
    src/di/Injector.cpp
    src/di/injector_rules.cpp
    src/msg/MessageQueue.cpp
    src/msg/MessageQueueFactory.cpp
    src/msg/MailboxEndpoint.cpp
    src/msg/MailboxRouter.cpp
    src/msg/MailboxSender.cpp
    src/msg/MailboxReceiver.cpp
    src/util/exception_handler.cpp
    src/util/CLIInput.cpp
    src/util/CLIParser.cpp
    ${PLATFORM_DIR}/KillableThread.cpp
    ${PLATFORM_DIR}/PrimitiveIO.cpp
    ${PLATFORM_DIR}/SystemWindow.cpp
)

if(MSVC)
  list(
    APPEND
    OML_SOURCE_FILE_MANIFEST
      ${PLATFORM_DIR}/winmain.cpp

      # For Windows targets, manifest files can be added as source files and will be embedded
      # in the resulting binary (https://cmake.org/cmake/help/v3.4/release/3.4.html#other)
      ${PLATFORM_DIR}/win32app.manifest
  )
endif()
