#include "omulator/msg/MailboxRouter.hpp"

#include "omulator/msg/MessageBuffer.hpp"
#include "omulator/msg/Package.hpp"
#include "omulator/util/ObjectPool.hpp"

#include <cassert>
#include <stdexcept>
#include <utility>

namespace omulator::msg {

MailboxRouter::MailboxRouter(ILogger &logger, di::Injector &injector)
  : logger_(logger), injector_(injector) { }

Mailbox &MailboxRouter::claim_mailbox(const di::Hash_t mailbox_hsh) {
  std::scoped_lock lck(mtx_);

  MailboxEntry &entry = get_entry_(mailbox_hsh);

  if(entry.claimed) {
    // Not necessarily a fatal error, but certainly an antipattern. This can result in dropped
    // messages (e.g. when two different objects call recv() on the same mailbox), but should not
    // cause undefined behavior, as Mailboxes themselves are threadsafe.
    logger_.error("Attempting to claim mailbox that has already been claimed.");
  }

  entry.claimed = true;
  return entry.mailbox;
}

Mailbox &MailboxRouter::get_mailbox(const di::Hash_t mailbox_hsh) {
  std::scoped_lock lck(mtx_);

  return get_entry_(mailbox_hsh).mailbox;
}

MailboxRouter::MailboxEntry &MailboxRouter::get_entry_(const di::Hash_t mailbox_hsh) {
  auto entry = mailboxes_.find(mailbox_hsh);

  if(entry == mailboxes_.end()) {
    auto newEntry = mailboxes_.emplace(std::make_pair(
      mailbox_hsh,
      std::make_unique<MailboxEntry>(logger_,
                                     injector_.get<util::ObjectPool<Package>>(),
                                     injector_.get<util::ObjectPool<MessageBuffer>>())));

    // Ensure that the emplacement was successful
    assert(newEntry.second);

    return *(newEntry.first->second);
  }
  else {
    return *(entry->second);
  }
}

MailboxRouter::MailboxEntry::MailboxEntry(ILogger &                        logger,
                                          util::ObjectPool<Package> &      pkgPool,
                                          util::ObjectPool<MessageBuffer> &mbPool)
  : mailbox(logger, pkgPool, mbPool), claimed(false) { }

}  // namespace omulator::msg
