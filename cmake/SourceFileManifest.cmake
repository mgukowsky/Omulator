set(
  OML_SOURCE_FILE_MANIFEST
    src/main.cpp
    src/di/Injector.cpp
    src/di/injector_rules.cpp
    src/msg/Mailbox.cpp
    src/msg/MailboxRouter.cpp
    src/msg/MessageBuffer.cpp
    src/msg/Package.cpp
    src/scheduler/Worker.cpp
    src/scheduler/Scheduler.cpp
    src/SpdlogLogger.cpp
    src/util/exception_handler.cpp
    ${PLATFORM_DIR}/CPUIdentifier.cpp
    ${PLATFORM_DIR}/PrimitiveIO.cpp
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
