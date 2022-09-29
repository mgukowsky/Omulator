#pragma once

#include "omulator/msg/MailboxEndpoint.hpp"

namespace omulator::msg {

/**
 * Lightweight wrapper around MailboxEndpoint which only allows for receiving messages.
 */
class MailboxReceiver {
public:
  explicit MailboxReceiver(MailboxEndpoint &endpoint);

  void recv(const MessageCallback_t &callback, RecvBehavior recvBehavior = RecvBehavior::BLOCK);

private:
  MailboxEndpoint &endpoint_;
};
}  // namespace omulator::msg
