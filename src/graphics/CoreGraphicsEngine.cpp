#include "omulator/graphics/CoreGraphicsEngine.hpp"

#include "omulator/util/TypeString.hpp"

namespace omulator::graphics {
CoreGraphicsEngine::CoreGraphicsEngine(di::Injector &injector)
  : Subsystem(injector.get<ILogger>(),
              util::TypeString<CoreGraphicsEngine>,
              injector.get<msg::MailboxRouter>(),
              util::TypeHash<CoreGraphicsEngine>),
    injector_(injector),
    logger_(injector_.get<ILogger>()),
    graphicsBackend_(injector_.get<IGraphicsBackend>()) {
  receiver_.on(msg::MessageType::RENDER_FRAME, [this] { graphicsBackend_.render_frame(); });
  receiver_.on(msg::MessageType::HANDLE_RESIZE, [this] { graphicsBackend_.handle_resize(); });
  receiver_.on_managed_payload<std::string>(
    msg::MessageType::SET_VERTEX_SHADER,
    [this](const std::string &shader) { graphicsBackend_.set_vertex_shader(shader); });
}

}  // namespace omulator