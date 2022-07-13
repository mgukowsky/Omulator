#include "omulator/msg/MailboxRouter.hpp"

#include <stdexcept>
#include <utility>

namespace omulator::msg {

MailboxRouter::MailboxRouter(ILogger &logger, MessageQueueFactory &mqfactory)
  : logger_(logger), mqfactory_(mqfactory) { }

MailboxReceiver MailboxRouter::claim_mailbox(const MailboxToken_t mailbox_hsh) {
  std::scoped_lock lck(mtx_);

  MailboxEndpoint &entry = get_entry_(mailbox_hsh);

  if(entry.claimed()) {
    throw std::runtime_error("Attempting to claim mailbox that has already been claimed!");
  }

  entry.claim();
  return MailboxReceiver{entry};
}

MailboxSender MailboxRouter::get_mailbox(const MailboxToken_t mailbox_hsh) {
  std::scoped_lock lck(mtx_);

  return MailboxSender{get_entry_(mailbox_hsh)};
}

MailboxEndpoint &MailboxRouter::get_entry_(const MailboxToken_t mailbox_hsh) {
  auto entry = mailboxes_.find(mailbox_hsh);

  if(entry == mailboxes_.end()) {
    auto newEntry = mailboxes_.try_emplace(mailbox_hsh, mailbox_hsh, mqfactory_);

    if(!newEntry.second) {
      throw std::runtime_error("Could not create MailboxEndpoint!");
    }

    return newEntry.first->second;
  }
  else {
    return entry->second;
  }
}

}  // namespace omulator::msg
