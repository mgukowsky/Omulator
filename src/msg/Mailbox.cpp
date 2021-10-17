#include "omulator/msg/Mailbox.hpp"

#include <cassert>

namespace omulator::msg {

Mailbox::Mailbox(ILogger &                        logger,
                 util::ObjectPool<Package> &      pool,
                 util::ObjectPool<MessageBuffer> &mbpool)
  : logger_(logger), pool_(pool), mbpool_(mbpool) { }

Package *Mailbox::open_pkg() {
  Package *pPkg = pool_.get();
  pPkg->reset(&mbpool_, &logger_);
  return pPkg;
}

void Mailbox::send(const Package *pPkg) {
  std::scoped_lock lck(mtx_);
  pkgQueue_.push(pPkg);
}

void Mailbox::recv(const ReceiverMap_t &receiverMap) {
  while(true) {
    const Package *pPkg = nullptr;
    {
      std::scoped_lock lck(mtx_);
      if(pkgQueue_.empty()) {
        break;
      }
      pPkg = pkgQueue_.front();
      pkgQueue_.pop();
    }

    pPkg->receive_msgs(receiverMap);

    Package *pEmptyPkg = const_cast<Package *>(pPkg);

    pEmptyPkg->release();
    pool_.return_to_pool(pEmptyPkg);
  }
}

}  // namespace omulator::msg
