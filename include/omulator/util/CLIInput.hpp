#pragma once

#include "omulator/msg/MailboxRouter.hpp"
#include "omulator/msg/MailboxSender.hpp"
#include "omulator/util/KillableThread.hpp"

#include <thread>

namespace omulator::util {
class CLIInput {
public:
  explicit CLIInput(msg::MailboxRouter &mbrouter);

  void input_loop();

private:
  msg::MailboxSender msgSender_;
  KillableThread     thrd_;
};
}  // namespace omulator::util