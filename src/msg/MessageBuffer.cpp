#include "omulator/msg/MessageBuffer.hpp"

#include <limits>
#include <stdexcept>

namespace omulator::msg {

void *MessageBuffer::alloc(const U32 id, const std::size_t size) {
  if(size > MAX_MSG_SIZE) {
    throw std::runtime_error("Requested storage greater than MAX_MSG_SIZE");
  }
  else if(can_alloc_(size)) {
    MessageHeader *hdr = reinterpret_cast<MessageHeader *>(buff_ + offsetLast_);
    hdr->id            = id;

    U16 nextHdr     = size + HEADER_SIZE;
    hdr->offsetNext = nextHdr;
    offsetLast_ += nextHdr;

    // Pointer arithmetic magic will point us to right after the storage for the message header.
    return hdr + 1;
  }
  else {
    return nullptr;
  }
}

const MessageBuffer::MessageHeader *MessageBuffer::begin() const noexcept {
  if(empty()) {
    return nullptr;
  }
  else {
    return reinterpret_cast<const MessageBuffer::MessageHeader *>(buff_ + offsetFirst_);
  }
}

bool MessageBuffer::empty() const noexcept {
  return offsetLast_ == 0 && offsetFirst_ == offsetLast_;
}

const MessageBuffer::MessageHeader *MessageBuffer::end() const noexcept {
  if(empty()) {
    return nullptr;
  }
  else {
    return reinterpret_cast<const MessageBuffer::MessageHeader *>(buff_ + offsetLast_);
  }
}

MessageBuffer MessageBuffer::make_buff() {
  MessageBuffer mb;
  mb.reset();
  return mb;
}

MessageBuffer *MessageBuffer::next_buff() const noexcept { return pNext_; }

MessageBuffer *MessageBuffer::next_buff(MessageBuffer *pNext) noexcept {
  pNext_ = pNext;
  return next_buff();
}

U16 MessageBuffer::offsetFirst() const noexcept { return offsetFirst_; }

U16 MessageBuffer::offsetLast() const noexcept { return offsetLast_; }

void MessageBuffer::reset() noexcept {
  pNext_       = nullptr;
  offsetFirst_ = 0;
  offsetLast_  = 0;

  // This static assert can really go anywhere; this seems like as good a place as any...
  static_assert(BUFFER_SIZE < std::numeric_limits<decltype(offsetLast_)>::max(),
                "MessageBuffer's members containing offsets to its internal buffer must be large "
                "enough to span the entire range of the internal buffer");
}

bool MessageBuffer::can_alloc_(const std::size_t size) const noexcept {
  if(size > MAX_MSG_SIZE) {
    return false;
  }
  else {
    return (size + HEADER_SIZE) <= (BUFFER_SIZE - offsetLast_);
  }
}

}  // namespace omulator::msg
