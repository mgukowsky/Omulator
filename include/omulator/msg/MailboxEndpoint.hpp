#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/msg/MessageQueue.hpp"
#include "omulator/msg/MessageQueueFactory.hpp"
#include "omulator/util/Pimpl.hpp"

#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <queue>

namespace omulator::msg {

/**
 * Used to determine how MailboxEndpoint::recv() should behave. If BLOCK, then recv will block
 * (potentially forever) until a message is sent to the associated MailboxEndpoint. Otherwise, if
 * NONBLOCK, then recv will not block and will simply return if no messages have been sent. Pending
 * messages are always processed in either case.
 */
enum class RecvBehavior : bool { BLOCK, NONBLOCK };

/**
 * An endpoint which acts as a sink for MessageQueues delivered to a given ID, which can then be
 * read by a consumer. Entirely threadsafe.
 */
class MailboxEndpoint {
public:
  /**
   * Initializes a mailbox in an unclaimed state.
   *
   * N.B. that the id provided here is purely for reference; it is not checked anywhere within this
   * class's internal logic.
   */
  MailboxEndpoint(const U64 id, ILogger &logger, MessageQueueFactory &mqfactory);

  /**
   * Destroys any pending MessageQueues; N.B. any messages in them will be dropped.
   */
  ~MailboxEndpoint();

  /**
   * Claim a given mailbox. MailboxEndpoint instances cannot be claimed more than once; they can
   * never be un-claimed.
   */
  void claim() noexcept;

  /**
   * Returns whether or not the mailbox is claimed.
   */
  bool claimed() const noexcept;

  /**
   * Removes a callback associated with a given MessageType via a call to on(). If no such callback
   * has previously been registered, then this function has no effect.
   */
  void off(const MessageType type);

  /**
   * Returns a fresh MessageQueue from the internal MessageQueueFactory, which can be filled with
   * messages and then sent via a call to send().
   */
  MessageQueue get_mq() noexcept;

  /**
   * Register a callback to be invoked when a given MessageType is processed. If there is already a
   * callback associated with the given MessageType, then this function has no effect.
   */
  void on(const MessageType type, const MessageCallback_t &callback);

  /**
   * For each message in each MessageQueue that has been sent, a matching callback registered with
   * on() (if one exists) will be invoked with the corresponding message payload.
   *
   * BLOCKS until messages are sent via a call to send(), unless RecvBehavior::NONBLOCK is provided
   * as the recvBehavior.
   */
  void recv(RecvBehavior recvBehavior = RecvBehavior::BLOCK);

  /**
   * Submit a MessageQueue to this endpoint, which can then be serviced via a call to recv(). seal()
   * will be called on the MessageQueue prior to submission.
   */
  void send(MessageQueue &mq);

private:
  const U64        id_;
  std::atomic_bool claimed_;

  ILogger             &logger_;
  MessageQueueFactory &mqfactory_;

  /**
   * Stores callbacks added by on().
   */
  std::map<MessageType, MessageCallback_t> callbacks_;

  /**
   * The underlying queue of MessageQueues. Accesses should be guarded by a mtx_.
   */
  std::queue<MessageQueue> queue_;

  std::condition_variable cv_;
  std::mutex              mtx_;
};

}  // namespace omulator::msg
