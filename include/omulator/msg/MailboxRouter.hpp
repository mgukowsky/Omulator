#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/di/Injector.hpp"
#include "omulator/msg/Mailbox.hpp"
#include "omulator/msg/MessageBuffer.hpp"
#include "omulator/msg/Package.hpp"
#include "omulator/util/ObjectPool.hpp"

#include <map>
#include <memory>
#include <mutex>

namespace omulator::msg {

class MailboxRouter {
public:
  MailboxRouter(ILogger &logger, di::Injector &injector);

  /**
   * Retrieves the mailbox corresponding to the given hash, creating the mailbox if it doesn't
   * exist. The mailbox is also claimed, and if the mailbox has already been claimed, then an error
   * will be thrown.
   *
   * LOCKS mtx_.
   */
  Mailbox &claim_mailbox(const di::Hash_t mailbox_hsh);

  /**
   * Same as claim_mailbox, except neither claims nor checks ownership.
   *
   * LOCKS mtx_.
   */
  Mailbox &get_mailbox(const di::Hash_t mailbox_hsh);

private:
  struct MailboxEntry {
    MailboxEntry(ILogger &                        logger,
                 util::ObjectPool<Package> &      pkgPool,
                 util::ObjectPool<MessageBuffer> &mbPool);

    Mailbox mailbox;
    bool    claimed;
  };

  /**
   * Abstracts shared logic between claim_mailbox and get_mailbox; assumes that another method has
   * already locked mtx_!
   */
  MailboxEntry &get_entry_(const di::Hash_t mailbox_hsh);

  ILogger &logger_;

  di::Injector &injector_;

  std::map<const di::Hash_t, std::unique_ptr<MailboxEntry>> mailboxes_;

  std::mutex mtx_;
};

}  // namespace omulator::msg
