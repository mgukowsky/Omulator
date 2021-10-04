#include "omulator/msg/Package.hpp"

#include <cassert>

namespace omulator::msg {

Package::Package(Pool_t &pool) : pool_(pool), head_(*(pool_.get())), current_(&head_) {
  current_->reset();
}

void Package::alloc_msg(const U32 id) {
  // The underlying call to MessageBuffer::alloc will return a pointer the MessageHeader, since
  // there is no associated data with this message. We have no need to expose this, so we choose not
  // to return it here.
  alloc_(id, 0);
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
  void *retval = nullptr;

  if(size == 0) {
    retval = current_->alloc(id);
  }
  else {
    retval = current_->alloc(id, size);
  }

  assert(retval != nullptr);

  return retval;
}

}  // namespace omulator::msg
