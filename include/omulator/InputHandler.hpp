#pragma once

#include "omulator/msg/MailboxRouter.hpp"

/**
 * Used to handle input events in a platform-agnostic manner. Platform-specific logic should convert
 * any events to an InputHandler::InputEvent and pass along the event to a member function of this
 * class.
 */
namespace omulator {
class InputHandler {
public:
  /**
   * Platform-agnostic input events.
   */
  enum class InputEvent { QUIT };

  explicit InputHandler(msg::MailboxRouter &mbrouter);

  void handle_input(const InputEvent input);

private:
  /**
   * Used to send messages to the main App instance.
   */
  msg::MailboxSender appSender_;
};
}  // namespace omulator
