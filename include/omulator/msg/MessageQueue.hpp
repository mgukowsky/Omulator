#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/msg/Message.hpp"

#include <functional>
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
  MessageQueue(ILogger &logger);

  /**
   * The number of MSG_NULL messages received since the queue was last unsealed.
   */
  std::size_t num_null_msgs() const noexcept;

  /**
   * Invoke a callback for each message in the queue. If the queue is not sealed, then no processing
   * will take place.
   */
  void pump_msgs(const MessageCallback_t &callback);

  /**
   * Overload which pushes a message with no payload.
   */
  void push(const MessageType type) noexcept;

  /**
   * Create a new message in the queue by copying in type and payload. If the queue has already been
   * sealed, then no message is created.
   */
  void push(const MessageType type, const U64 payload) noexcept;

  /**
   * Unseal the queue and reset its contents. Does NOT release any underlying memory.
   */
  void reset() noexcept;

  /**
   * Seal the queue. If already sealed, does nothing.
   */
  void seal() noexcept;

private:
  std::vector<Message> queue_;

  ILogger &logger_;

  bool sealed_;

  std::size_t numNulls_;
};
}  // namespace omulator::msg
