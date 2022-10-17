#include "omulator/InputHandler.hpp"

#include "omulator/App.hpp"
#include "omulator/TestGraphicsEngine.hpp"

namespace omulator {

InputHandler::InputHandler(msg::MailboxRouter &mbrouter)
  : appSender_(mbrouter.get_mailbox<App>()),
    graphicsSender_(mbrouter.get_mailbox<TestGraphicsEngine>()) { }

void InputHandler::handle_input(const InputEvent input) {
  switch(input) {
    case InputEvent::QUIT:
      appSender_.send_single_message(msg::MessageType::APP_QUIT);
      break;
    case InputEvent::RESIZE:
      graphicsSender_.send_single_message(msg::MessageType::HANDLE_RESIZE);
      break;
    default:
      break;
  }
}
}  // namespace omulator