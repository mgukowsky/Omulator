#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/di/TypeHash.hpp"
#include "omulator/msg/MessageBuffer.hpp"
#include "omulator/util/ObjectPool.hpp"
#include "omulator/util/reinterpret.hpp"

#include <functional>
#include <map>
#include <type_traits>

namespace omulator::msg {

class Package {
public:
  using MsgReceiverFn_t = std::function<void(const void *)>;
  using Pool_t          = util::ObjectPool<MessageBuffer>;
  using ReceiverMap_t   = std::map<U32, MsgReceiverFn_t>;

  Package(Pool_t &pool, ILogger &logger);
  ~Package();

  Package(const Package &) = delete;
  Package &operator=(const Package &) = delete;
  Package(Package &&)                 = delete;
  Package &operator=(Package &&) = delete;

  template<typename Raw_t, typename T = std::decay_t<Raw_t>>
  T &alloc_data() {
    // Fairly self-evident assert, so we can omit the explanation string.
    static_assert(sizeof(T) <= MessageBuffer::MAX_MSG_SIZE);

    static_assert(std::is_trivial_v<T>,
                  "A Package can only allocate messages which are of a trivial type, since neither "
                  "a constructor nor a destructor for said type will necessarily be invoked");

    static_assert(
      sizeof(T) > 0,
      "Package::alloc_data() is not intended only for types that have size > 0 and contain data. "
      "Consider using Package::alloc_msg for messages that do not contain any associated data.");

    return util::reinterpret<T>(alloc_(di::TypeHash32<T>, sizeof(T)));
  }

  void alloc_msg(const U32 id);

  /**
   * Walk through the messages in the Package in the order they appear in the Package. For each
   * message in the Package, if there is a matching receiver function in the receiverMap, then
   * the receiver function will be invoked. If invoked, the receiver function will receive a
   * pointer to the message data, or a nullptr if there is no data associated with the message.
   * If there is no matching receiver function in the receiver_map, then no callback function
   * will be invoked and a message will be logged indicating that there was a dropped message.
   *
   * I personally prefer this interface as it means we don't need to provide any of the underlying
   * MessageBuffer's internals outside this class, and it gives us the ability to easily record when
   * messages would be dropped.
   */
  void receive_msgs(const ReceiverMap_t &receiver_map);

private:
  Pool_t &       pool_;
  ILogger &      logger_;
  MessageBuffer &head_;
  MessageBuffer *pCurrent_;

  void *alloc_(const U32 id, const MessageBuffer::Offset_t size);
  void *try_alloc_(const U32 id, const MessageBuffer::Offset_t size);
};

}  // namespace omulator::msg
