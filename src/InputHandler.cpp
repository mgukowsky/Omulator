#include "omulator/InputHandler.hpp"

#include "omulator/App.hpp"

namespace omulator {

InputHandler::InputHandler(msg::MailboxRouter &mbrouter)
  : appSender_(mbrouter.get_mailbox<App>()) { }

void InputHandler::handle_input(const InputEvent input) {
  switch(input) {
    case InputEvent::QUIT:
      appSender_.send_single_message(msg::MessageType::APP_QUIT);
      break;
    default:
      break;
  }
}
}  // namespace omulator