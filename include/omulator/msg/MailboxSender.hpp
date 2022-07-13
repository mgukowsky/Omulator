#pragma once

#include "omulator/msg/MailboxEndpoint.hpp"

namespace omulator::msg {

/**
 * Lightweight around MailboxEndpoint which only allows for acquiring MessageQueues and sending
 * them.
 */
class MailboxSender {
public:
  MailboxSender(MailboxEndpoint &endpoint);

  MessageQueue *get_mq() noexcept;
  void          send(MessageQueue *pmq);

private:
  MailboxEndpoint &endpoint_;
};
}  // namespace omulator::msg
