#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/msg/Package.hpp"
#include "omulator/util/ObjectPool.hpp"

#include <memory>
#include <mutex>
#include <queue>

namespace omulator::msg {

class Mailbox {
public:
  Mailbox(ILogger &                        logger,
          util::ObjectPool<Package> &      pool,
          util::ObjectPool<MessageBuffer> &mbpool);

  /**
   * Get a fresh package to which messages can now be added. Internally, Package::reset() will be
   * called on the returned instance.
   *
   * N.B. that failing to call Package::send with the Package returned by this function constitutes
   * a resource leak.
   *
   * Does not lock mtx_, as the underlying ObjectPool methods are atomic.
   */
  [[nodiscard]] Package *open_pkg();

  /**
   * Enqueue a Package to be received by this mailbox. N.B. that pkg should have been acquired from
   * this same Mailbox via a call to open_pkg().
   *
   * LOCKS mtx_.
   */
  void send(const Package *pPkg);

  /**
   * Respond to messages in queued packages. Blocks until all messages in the queue have been
   * responded to. Once all messages in a Package have been received, the Package::release() will be
   * called on the Package, and it will be returned to pool_.
   *
   * Periodically LOCKS mtx_ to pull a package off of the queue.
   */
  std::size_t recv(const ReceiverMap_t &receiverMap);

private:
  ILogger &                  logger_;
  util::ObjectPool<Package> &pool_;

  /**
   * N.B. that this is not used directly by this class, but is passed to Packages.
   */
  util::ObjectPool<MessageBuffer> &mbpool_;

  std::queue<const Package *> pkgQueue_;
  std::mutex                  mtx_;
};

}  // namespace omulator::msg
