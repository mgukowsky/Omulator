#pragma once

#include "omulator/msg/MailboxRouter.hpp"

namespace omulator {
class InputHandler {
public:
  enum class InputEvent { QUIT };

  explicit InputHandler(msg::MailboxRouter &mbrouter);

  void handle_input(const InputEvent input);

private:
  msg::MailboxSender appSender_;
};
}  // namespace omulator