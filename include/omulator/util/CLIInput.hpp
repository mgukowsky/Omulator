#pragma once

#include "omulator/msg/MailboxRouter.hpp"
#include "omulator/msg/MailboxSender.hpp"
#include "omulator/util/KillableThread.hpp"

#include <thread>

/**
 * Class which watches for input on STDIN and sends a message each time a string is received.
 *
 * TODO: there is no clean way to perform a nonblocking equivalent to std::getline in a platform
 * agnostic manner, hence the need to KillableThread to force the thread waiting on std::getline to
 * terminate upon destruction. A cleaner design would be to utilize a console implemented in ImGui
 * for user input, and allow a socket to be opened for remote input which would be monitored
 * asynchronously (e.g. for input during CI testing).
 */
namespace omulator::util {
class CLIInput {
public:
  explicit CLIInput(msg::MailboxRouter &mbrouter);

  /**
   * Monitors stdin and sends a message each time a string is received; does not return.
   */
  void input_loop();

private:
  msg::MailboxSender msgSender_;
  KillableThread     thrd_;
};
}  // namespace omulator::util
