#pragma once

#include "omulator/msg/MailboxEndpoint.hpp"

#include <type_traits>

namespace omulator::msg {

/**
 * Lightweight wrapper around MailboxEndpoint which only allows for sending MessageQueues.
 */
class MailboxSender {
public:
  explicit MailboxSender(MailboxEndpoint &endpoint);

  MessageQueue get_mq() noexcept;
  void         send(MessageQueue &mq);

  /**
   * Convenience function to send a single message. Should not be used too often, as it is more
   * efficient to batch multiple messages into a single queue.
   */
  template<typename T = const U64>
  requires valid_trivial_payload_type<T>
  void send_single_message(const MessageType type, const T payload = T()) {
    MessageQueue mq = get_mq();
    mq.push(type, payload);
    send(mq);
  }

private:
  MailboxEndpoint &endpoint_;
};
}  // namespace omulator::msg
