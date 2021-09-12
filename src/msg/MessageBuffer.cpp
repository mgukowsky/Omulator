#include "omulator/msg/MessageBuffer.hpp"

#include <limits>
#include <stdexcept>

namespace omulator::msg {

void *MessageBuffer::alloc(const U32 id, const MessageBuffer::Offset_t size) {
  if(size > MAX_MSG_SIZE) {
    throw std::runtime_error("Requested storage greater than MAX_MSG_SIZE");
  }
  else if(can_alloc_(size)) {
    MessageHeader *hdr = reinterpret_cast<MessageHeader *>(buff_ + offsetLast_);
    hdr->id            = id;

    Offset_t nextHdr = size + HEADER_SIZE;
    hdr->offsetNext  = nextHdr;
    offsetLast_ += nextHdr;

    // Pointer arithmetic magic will point us to right after the storage for the message header.
    return hdr + 1;
  }
  else {
    return nullptr;
  }
}

void *MessageBuffer::alloc(const U32 id) noexcept {
  std::byte *pAlloc = reinterpret_cast<std::byte *>(alloc(id, 0));
  if(pAlloc != nullptr) {
    pAlloc -= HEADER_SIZE;
  }

  return pAlloc;
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

MessageBuffer::Offset_t MessageBuffer::offsetFirst() const noexcept { return offsetFirst_; }

MessageBuffer::Offset_t MessageBuffer::offsetLast() const noexcept { return offsetLast_; }

void MessageBuffer::reset() noexcept {
  pNext_       = nullptr;
  offsetFirst_ = 0;
  offsetLast_  = 0;

  // This static assert can really go anywhere; this seems like as good a place as any...
  static_assert(BUFFER_SIZE < std::numeric_limits<decltype(offsetLast_)>::max(),
                "MessageBuffer's members containing offsets to its internal buffer must be large "
                "enough to span the entire range of the internal buffer");

  // ditto, although arguably this is more of a nice-to-have than a strict requirement... may help
  // with alignment of allocations though!
  static_assert(BUFFER_SIZE % HEADER_SIZE == 0,
                "A MessageBuffer should be able to be filled exclusively with MessageHeaders with "
                "no space left over");
}

bool MessageBuffer::can_alloc_(const MessageBuffer::Offset_t size) const noexcept {
  if(size > MAX_MSG_SIZE) {
    return false;
  }
  else {
    return (size + HEADER_SIZE) <= (BUFFER_SIZE - offsetLast_);
  }
}

}  // namespace omulator::msg
