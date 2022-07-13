#pragma once

#include "omulator/msg/MailboxEndpoint.hpp"

namespace omulator::msg {

/**
 * Lightweight wrapper around MailboxEndpoint which only allows for receiving messages.
 */
class MailboxReceiver {
public:
  MailboxReceiver(MailboxEndpoint &endpoint);

  void recv(const MessageCallback_t &callback);

private:
  MailboxEndpoint &endpoint_;
};
}  // namespace omulator::msg
