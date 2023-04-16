#pragma once

#include "omulator/msg/MailboxEndpoint.hpp"

#include <type_traits>

namespace omulator::msg {

/**
 * Lightweight wrapper around MailboxEndpoint which only allows for receiving messages.
 */
class MailboxReceiver {
public:
  explicit MailboxReceiver(MailboxEndpoint &endpoint);

  /**
   * Various on*() methods. They all call endpoint_.on(), so note that calling more than one version
   * of an on*() method with the same message type (without calling off() in between) will have no
   * effect.
   */

  /**
   * Register a callback function which takes no arguments for a given message type.
   */
  void on(const MessageType type, std::function<void(void)> callback);

  /**
   * Register a callback function which takes a single by-value argument for a given message type.
   * The argument will be copied directly from the payload of the message.
   */
  template<typename T = U64>
  requires valid_trivial_payload_type<T>
  void on_trivial_payload(const MessageType type, std::function<void(const T)> callback) {
    if constexpr(std::is_pointer_v<T>) {
      endpoint_.on(type,
                   [callback](const Message &msg) { callback(reinterpret_cast<T>(msg.payload)); });
    }
    else {
      endpoint_.on(type, [callback](const Message &msg) { callback(static_cast<T>(msg.payload)); });
    }
  }

  /**
   * Register a callback function which takes a single by-reference argument for a given message
   * type. The argument will be dereferenced from the payload of the message, and is only guaranteed
   * to be a valid reference while the function is being invoked; i.e. do not propogate the
   * reference outside of this function.
   */
  template<typename T>
  void on_managed_payload(const MessageType type, std::function<void(const T &)> callback) {
    endpoint_.on(type, [callback](const Message &msg) { callback(msg.get_managed_payload<T>()); });
  }

  /**
   * Same as on_trivial_payload, but interprets the message payload as a pointer to a mutable
   * object. Useful for circumstances where the callback needs to mutate shared state, and should be
   * used sparingly.
   *
   * N.B. that this is different from and LESS SAFE than on_managed_payload in that the payload is
   * treated as mutable AND type checking is NOT performed.
   */
  template<typename T>
  void on_unmanaged_payload(const MessageType type, std::function<void(T &)> callback) {
    endpoint_.on(
      type, [callback](const Message &msg) { callback(*(reinterpret_cast<T *>(msg.payload))); });
  }

  void off(const MessageType type);

  void recv(RecvBehavior recvBehavior = RecvBehavior::BLOCK);

private:
  MailboxEndpoint &endpoint_;
};
}  // namespace omulator::msg
