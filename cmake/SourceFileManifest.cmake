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
    src/graphics/CoreGraphicsEngine.cpp
    src/msg/MessageQueue.cpp
    src/msg/MessageQueueFactory.cpp
    src/msg/MailboxEndpoint.cpp
    src/msg/MailboxRouter.cpp
    src/msg/MailboxSender.cpp
    src/msg/MailboxReceiver.cpp
    src/ui/ImGuiBackend.cpp
    src/util/exception_handler.cpp
    src/util/CLIInput.cpp
    src/util/CLIParser.cpp
    src/vkmisc/Allocator.cpp
    src/vkmisc/Frame.cpp
    src/vkmisc/Initializer.cpp
    src/vkmisc/Pipeline.cpp
    src/vkmisc/Shader.cpp
    src/vkmisc/SimpleMesh.cpp
    src/vkmisc/Swapchain.cpp
    src/vkmisc/vkmisc.cpp
    ${PLATFORM_DIR}/KillableThread.cpp
    ${PLATFORM_DIR}/PrimitiveIO.cpp
    ${PLATFORM_DIR}/SystemWindow.cpp
)

set(
  OML_SHADER_FILE_MANIFEST
    shaders/tri_mesh.frag
    shaders/red_triangle.vert
    shaders/triangle.vert
    shaders/tri_mesh.vert
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
