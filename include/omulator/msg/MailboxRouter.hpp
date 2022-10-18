#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/msg/MailboxReceiver.hpp"
#include "omulator/msg/MailboxSender.hpp"
#include "omulator/msg/MessageQueue.hpp"
#include "omulator/util/TypeHash.hpp"

#include <map>
#include <memory>
#include <mutex>
#include <type_traits>

namespace omulator::msg {

using MailboxToken_t = util::Hash_t;

class MailboxRouter {
public:
  MailboxRouter(ILogger &logger, MessageQueueFactory &mqfactory);

  /**
   * Claims the mailbox corresponding to the given hash, creating the mailbox if it doesn't
   * exist. If the mailbox has already been claimed, then an error will be thrown.
   *
   * LOCKS mtx_.
   */
  MailboxReceiver claim_mailbox(const MailboxToken_t mailbox_hsh);

  /**
   * Convenience overload to use a TypeHash.
   */
  template<typename Raw_t, typename T = std::remove_pointer_t<std::decay_t<Raw_t>>>
  MailboxReceiver claim_mailbox() {
    return claim_mailbox(util::TypeHash<T>);
  }

  /**
   * Same as claim_mailbox, except neither claims nor checks ownership, and returns a MailboxSender
   * instead of a MailboxReceiver.
   *
   * LOCKS mtx_.
   */
  MailboxSender get_mailbox(const MailboxToken_t mailbox_hsh);

  /**
   * Convenience overload to use a TypeHash.
   */
  template<typename Raw_t, typename T = std::remove_pointer_t<std::decay_t<Raw_t>>>
  MailboxSender get_mailbox() {
    return get_mailbox(util::TypeHash<T>);
  }

private:
  /**
   * Retrieves the MailboxEndpoint corresponding to the given token, and creates it if it doesn't
   * already exist.
   */
  MailboxEndpoint &get_entry_(const MailboxToken_t mailbox_hsh);

  ILogger             &logger_;
  MessageQueueFactory &mqfactory_;

  std::map<const MailboxToken_t, MailboxEndpoint> mailboxes_;

  std::mutex mtx_;
};

}  // namespace omulator::msg
