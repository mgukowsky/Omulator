#include "omulator/msg/Package.hpp"

namespace omulator::msg {

Package::Package(Pool_t &pool) : pool_(pool), head_(*(pool_.get())), current_(&head_) {
  current_->reset();
}

void *Package::try_alloc_(const U32 id, const MessageBuffer::Offset_t size) {
  return current_->alloc(id, size);
}

}  // namespace omulator::msg
