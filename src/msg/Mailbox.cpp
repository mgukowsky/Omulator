#include "omulator/msg/Mailbox.hpp"

#include <cassert>
#include <sstream>

namespace omulator::msg {

Mailbox::Mailbox(ILogger                         &logger,
                 util::ObjectPool<Package>       &pool,
                 util::ObjectPool<MessageBuffer> &mbpool)
  : logger_(logger), pool_(pool), mbpool_(mbpool) { }

void Mailbox::off(const U32 msg) {
  std::scoped_lock lck(mtx_);
  if(auto it = receiverMap_.find(msg); it != receiverMap_.end()) {
    receiverMap_.erase(it);
  }
}

void Mailbox::on(const U32 msg, MsgReceiverFn_t callback) {
  std::scoped_lock lck(mtx_);

  auto result = receiverMap_.emplace(msg, callback);
  if(!result.second) {
    std::stringstream ss;
    ss << "Attempted to add a callback for " << msg
       << " but there was a callback already registered with the mailbox";
    logger_.warn(ss.str().c_str());
  }
}

Package *Mailbox::open_pkg() {
  Package *pPkg = pool_.get();
  pPkg->reset(&mbpool_, &logger_);
  return pPkg;
}

void Mailbox::recv() {
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

    pPkg->receive_msgs(receiverMap_);

    Package *pEmptyPkg = const_cast<Package *>(pPkg);

    pEmptyPkg->release();
    pool_.return_to_pool(pEmptyPkg);
  }
}

void Mailbox::send(const Package *pPkg) {
  std::scoped_lock lck(mtx_);
  pkgQueue_.push(pPkg);
}

}  // namespace omulator::msg
