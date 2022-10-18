#include "omulator/CoreGraphicsEngine.hpp"

#include "omulator/util/TypeString.hpp"

namespace omulator {
CoreGraphicsEngine::CoreGraphicsEngine(di::Injector &injector)
  : Subsystem(injector.get<ILogger>(),
              util::TypeString<CoreGraphicsEngine>,
              injector.get<msg::MailboxRouter>(),
              util::TypeHash<CoreGraphicsEngine>),
    injector_(injector),
    logger_(injector_.get<ILogger>()),
    graphicsBackend_(injector_.get<IGraphicsBackend>()) {
  start_();
}

void CoreGraphicsEngine::message_proc(const msg::Message &msg) {
  if(msg.type == msg::MessageType::RENDER_FRAME) {
    graphicsBackend_.render_frame();
  }
  else if(msg.type == msg::MessageType::HANDLE_RESIZE) {
    graphicsBackend_.handle_resize();
  }
  else {
    Subsystem::message_proc(msg);
  }
}
}  // namespace omulator