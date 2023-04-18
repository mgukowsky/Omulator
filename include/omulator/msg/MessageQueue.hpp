#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/di/TypeMap.hpp"
#include "omulator/msg/Message.hpp"
#include "omulator/util/to_underlying.hpp"

#include <cassert>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

namespace omulator::msg {

/**
 * Callback used to process messages.
 */
using MessageCallback_t = std::function<void(const Message &)>;

/**
 * A queue containing a series of messages. Messages are pushed and popped in a FIFO manner. This
 * class is entirely unsynchronized and should only be accessed by a single thread at a given time.
 * At any given time, MessageQueues can be unsealed, meaning they can accept additional messages, or
 * sealed, meaning that they cannot accept any additional messages and are in a read-only state.
 * MessageQueues can be put back into an unsealed state for reuse, however doing so will result in
 * the loss of any messages currently in the queue. The internal implementation is designed to hold
 * on to any allocated memory until the queue is destroyed, making this class an ideal candidate for
 * re-use within a resource pool.
 */
class MessageQueue {
public:
  /**
   * Initialized the queue in an unsealed state.
   */
  explicit MessageQueue(ILogger &logger);

  /**
   * Invoke a callback for each message in the queue. If the queue is not sealed, then no
   * processing will take place.
   */
  void pump_msgs(const MessageCallback_t &callback);

  /**
   * Overload which pushes a message with no payload.
   */
  void push(const MessageType type) noexcept;

  /**
   * Create a new message in the queue by copying in type and payload. If the queue has already
   * been sealed, then no message is created.
   */
  template<typename T>
  requires valid_trivial_payload_type<T>
  void push(const MessageType type, const MessageFlagType mflags, const T payload) noexcept {
    static_assert(sizeof(T) <= sizeof(U64),
                  "Payload type must be <= sizeof(U64) in order to prevent data loss");

    if constexpr(std::is_pointer_v<T>) {
      push_impl_(type, mflags, reinterpret_cast<const U64>(payload));
    }
    else {
      push_impl_(type, mflags, static_cast<const U64>(payload));
    }
  }

  template<typename T>
  void push(const MessageType type, const T payload) {
    push(type, MessageFlagType::FLAGS_NULL, payload);
  }

  /**
   * Create a new instance of type T and push it as a message. Returns a reference that can be
   * used to manipulate the new instance. The new T instance will be entirely managed by the
   * MessageQueue instance.
   *
   * N.B. that there is NO NEED to call push for this message; it will already have been pushed
   * onto the queue by the time this function returns.
   *
   * Also N.B. that the new T instance will be DELETED once a call to pump_msgs dequeues this
   * message.
   */
  template<typename T, typename... Args>
  T &push_managed_payload(const MessageType type, Args &&...args) {
    auto pCtr = new di::TypeContainer<T>();
    pCtr->createInstance(std::forward<Args>(args)...);
    push(type, MessageFlagType::MANAGED_PTR, pCtr);
    return pCtr->ref();
  }

  /**
   * Unseal the queue and reset its contents. Does NOT release any underlying memory.
   */
  void reset() noexcept;

  /**
   * Seal the queue. If already sealed, does nothing.
   */
  void seal() noexcept;

  /**
   * Returns true if the queue is sealed.
   */
  bool sealed() const noexcept;

private:
  void push_impl_(const MessageType type, const MessageFlagType mflags, const U64 payload) noexcept;

  std::vector<Message> queue_;

  ILogger &logger_;

  bool sealed_;
};
}  // namespace omulator::msg
