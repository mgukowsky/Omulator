#pragma once

#include "omulator/msg/MessageQueue.hpp"
#include "omulator/msg/MessageQueueFactory.hpp"
#include "omulator/util/Pimpl.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>

namespace omulator::msg {

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
  MailboxEndpoint(const U64 id, MessageQueueFactory &mqfactory);

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
   * Returns a fresh MessageQueue from the internal MessageQueueFactory, which can be filled with
   * messages and then sent via a call to send().
   */
  MessageQueue *get_mq() noexcept;

  /**
   * Invokes the provided callback for each MessageQueue the endpoint has been provided via calls to
   * send(), in the order they were sent. Once all messages in a MessageQueue have been responded
   * to, the MessageQueue will be returned back to the MessageQueueFactory instance.
   *
   * BLOCKS until messages are sent via a call to send().
   */
  void recv(const MessageCallback_t &callback);

  /**
   * Submit a MessageQueue to this endpoint, which can then be serviced via a call to recv(). seal()
   * will be called on the MessageQueue prior to submission.
   */
  void send(MessageQueue *mq);

private:
  const U64        id_;
  std::atomic_bool claimed_;

  MessageQueueFactory &mqfactory_;

  /**
   * The underlying queue of MessageQueues. Accesses should be guarded by a mtx_.
   */
  std::queue<MessageQueue *> queue_;

  std::condition_variable cv_;
  std::mutex              mtx_;

  /**
   * Atomically retrieve the next message from the message queue. Returns a pointer to a
   * MessageQueue if one could be dequeued, and nullptr otherwise.
   */
  MessageQueue *pop_next_();
};

}  // namespace omulator::msg
