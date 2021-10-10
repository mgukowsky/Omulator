#include "omulator/msg/Package.hpp"

#include <cassert>

namespace omulator::msg {

using Hdr_t = MessageBuffer::MessageHeader;

Package::Package(Pool_t &pool, ILogger &logger)
  : pool_(pool), logger_(logger), head_(*(pool_.get())), current_(&head_) {
  current_->reset();
}

void Package::alloc_msg(const U32 id) {
  // The underlying call to MessageBuffer::alloc will return a pointer the MessageHeader, since
  // there is no associated data with this message. We have no need to expose this, so we choose not
  // to return it here.
  alloc_(id, 0);
}

void Package::receive_msgs(const Package::ReceiverMap_t &receiver_map) {
  const MessageBuffer *buff = &head_;
  const Hdr_t *        pHdr = buff->begin();

  while(pHdr != nullptr) {
    if(pHdr == buff->end()) {
      const MessageBuffer *nextBuff = buff->next_buff();
      if(nextBuff == nullptr) {
        break;
      }
      else {
        buff = nextBuff;
        pHdr = buff->begin();

        assert(pHdr != nullptr);
      }
    }

    const Hdr_t &hdr = *pHdr;

    if(receiver_map.contains(hdr.id)) {
      receiver_map.at(hdr.id)(hdr.data());
    }
    else {
      std::string errmsg("Could not find receiver for type: ");
      errmsg += std::to_string(hdr.id);
      logger_.warn(errmsg.c_str());
    }

    pHdr =
      reinterpret_cast<const Hdr_t *>(reinterpret_cast<const std::byte *>(pHdr) + hdr.offsetNext);
  }
}

void *Package::alloc_(const U32 id, const MessageBuffer::Offset_t size) {
  void *pAlloc = try_alloc_(id, size);

  // If the allocation fails then we have to acquire a new MessageBuffer
  if(pAlloc == nullptr) {
    auto *pNextBuff = pool_.get();
    current_->next_buff(pNextBuff);
    current_ = current_->next_buff();
    current_->reset();

    // Unlike the previous attempt to set pAlloc, this invocation cannot fail since we have the
    // static assert for MAX_MSG_SIZE above and we are allocating from an empty MessageBuffer.
    pAlloc = try_alloc_(id, size);
  }

  return pAlloc;
}

void *Package::try_alloc_(const U32 id, const MessageBuffer::Offset_t size) {
  if(size == 0) {
    return current_->alloc(id);
  }
  else {
    return current_->alloc(id, size);
  }
}

}  // namespace omulator::msg
