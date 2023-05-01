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
 *
 * At any given time, MessageQueues can be unsealed, meaning they can accept additional messages, or
 * sealed, meaning that they cannot accept any additional messages and are in a read-only state.
 * Marking a queue as sealed is a one-way transformation; once sealed it cannot be unsealed.
 *
 * Queues start out in a valid state (meaning that their internal storage is safe for reading and
 * writing) but become invalid once they relinquish their storage via a call to release() or a call
 * to mark_invalid(). This is also a one-way transformation, and there is no way for an invalid
 * queue to become valid again. Once a queue is marked invalid, most operations performed on it will
 * no longer succeed.
 *
 * N.B. that this class is meant to be lightweight and as such does not manage its own storage; this
 * allows it to be efficiently passed by value regardless of how large its storage grows.
 */
class MessageQueue {
public:
  struct Storage_t {
    explicit Storage_t(const U64 idArg) : id{idArg} { }

    const U64            id;
    std::vector<Message> storage;
  };

  /**
   * Initialized the queue in an unsealed state.
   */
  MessageQueue(Storage_t *pStorage, ILogger &logger);

  ~MessageQueue() noexcept = default;

  // TODO: do we want to mark a copied-from MessageQueue as invalid? We need the copy ctors in order
  // to play nicely with the STL containers which use them, however lifetime issues could arise
  // since both MessageQueues could be valid and refer to the same storage, and one could
  // potentially outlive the other and still reference the storage... for now best to advise not to
  // copy if possible, and if copying must happen, to not use the copied-from instance for
  // anything...
  MessageQueue(const MessageQueue &)            = default;
  MessageQueue &operator=(const MessageQueue &) = default;

  // N.B. that moved-from MessageQueues will no longer be valid!
  MessageQueue(MessageQueue &&rhs) noexcept;
  MessageQueue &operator=(MessageQueue &&rhs) noexcept;

  /**
   * Free any managed payloads in the queue, and then seals it. Has no effect if called more than
   * once.
   */
  void clear();

  /**
   * Set valid_ to false.
   */
  void mark_invalid() noexcept;

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
    if(!valid_) {
      // If we don't throw here, then we will get a memory leak/undefined behavior via the return
      // value since pCtr will be created but not added as a managed payload since the call to
      // push() will fail.
      throw std::runtime_error(
        "Attempted to call MessageQueue::pump_msgs() on a MessageQueue that is not valid");
    }
    auto pCtr = new di::TypeContainer<T>();
    pCtr->createInstance(std::forward<Args>(args)...);
    push(type, MessageFlagType::MANAGED_PTR, pCtr);
    return pCtr->ref();
  }

  /**
   * Retrieve a pointer to the internal storage and release ownership over the storage by marking
   * the MessageQueue as invalid.
   */
  Storage_t *release() noexcept;

  /**
   * Seal the queue. If already sealed, does nothing.
   */
  void seal() noexcept;

  /**
   * Returns true if the queue is sealed.
   */
  bool sealed() const noexcept;

  /**
   * Returns true if the queue contains valid storage.
   */
  bool valid() const noexcept;

private:
  /**
   * Leverage the type erasure we get from TypeContainer to delete a managed payload.
   */
  static void free_managed_payload_(Message &msg);

  void push_impl_(const MessageType type, const MessageFlagType mflags, const U64 payload) noexcept;

  /**
   * Pointer to externally managed storage.
   */
  Storage_t *pStorage_;

  ILogger &logger_;

  /**
   * Indicates that the queue can accept additional messages.
   */
  bool sealed_;

  /**
   * Indicates that the queue's internal storage is valid.
   */
  bool valid_;
};
}  // namespace omulator::msg
