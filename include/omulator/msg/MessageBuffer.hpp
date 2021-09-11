#pragma once

#include "omulator/oml_types.hpp"

namespace omulator::msg {

/**
 * A simple data structure used to hold messages of various sizes. Designed to be optimal in terms
 * of space and data locality.
 *
 * While MessageBuffers are designed to record and deliver messages in a guaranteed order, they only
 * retain a bare amount of meta-information in order to limit the responsibilities of this class.
 * Bookkeeping functionality such as iterators should be handled externally to this class.
 */
class MessageBuffer {
public:
  /**
   * A control structure containing meta-information about the message which directly proceeds it.
   */
  struct MessageHeader {
    /**
     * A magic number used to identify the message (e.g. its type, sequence, etc.). Need not be
     * unique.
     */
    U32 id;

    /**
     * Offset of the next MessageHeader from the start of this struct (i.e. inclusive of
     * HEADER_SIZE).
     */
    U16 offsetNext;
    U16 reserved_;
  };

  /**
   * Sets up a message header and returns the storage after the header. Returns nullptr if a new
   * buffer needs to be allocated.
   *
   * Will THROW if size exceeds MAX_MSG_SIZE!
   *
   * @param id An identifier for the message, which need not be unique
   * @param size The size, in bytes, to allocate within the MessageBuffer
   */
  void *alloc(const U32 id, const std::size_t size);

  /**
   * Return the first MessageHeader in the buffer, or nullptr if it's empty.
   */
  const MessageHeader *begin() const noexcept;

  bool empty() const noexcept;

  /**
   * Returns one past the last MessageHeader in the buffer (i.e. DO NOT dereference the return
   * value!), or nullptr if it's empty.
   */
  const MessageHeader *end() const noexcept;

  MessageBuffer *next_buff() const noexcept;
  MessageBuffer *next_buff(MessageBuffer *pNext) noexcept;

  U16 offsetFirst() const noexcept;
  U16 offsetLast() const noexcept;

  /**
   * MessageBuffer needs to be a trivial type since it is intended to be resused (e.g. as part of an
   * ObjectPool), so we settle on two-phase construction with this function to revert this object to
   * an unitialized state.
   *
   * N.B. that this will reset pNext_ to a nullptr, thereby causing a MEMORY LEAK if this function
   * is not used with care!
   */
  void reset() noexcept;

  /**
   * The size of the buffer is somewhat arbitrary, but we want it to be somewhat cache-friendly. We
   * use a buffer of 8K, or roughly 2 standard pages on x64.
   */
  static constinit const std::size_t BUFFER_SIZE  = 0x2000;
  static constinit const std::size_t HEADER_SIZE  = sizeof(MessageHeader);
  static constinit const std::size_t MAX_MSG_SIZE = BUFFER_SIZE - sizeof(MessageHeader);

  /**
   * Factory convenience function to create and initialize a MessageBuffer.
   */
  static MessageBuffer make_buff();

private:
  bool can_alloc_(const std::size_t size) const noexcept;

  /**
   * The encapsulated buffer. N.B. this is not allocated on the separately on the heap; it is part
   * of the object itself.
   *
   * TODO: Should be aligned to page size.
   */
  U8 buff_[BUFFER_SIZE];

  /**
   * A pointer to the next MessageBuffer which contains messages subsequent to the ones stored in
   * this buffer, or nullptr if there are no such subsequent messages.
   */
  MessageBuffer *pNext_;

  /**
   * Distance in bytes from the start of buff_ to the location within buff_ of the first
   * MessageHeader.
   */
  U16 offsetFirst_;

  /**
   * Distance in bytes from the start of buff_ to the location within buff_ where the next
   * MessageHeader can be placed.
   *
   * N.B. that this should not be dereferenced directly; similar to a std::end iterator this may
   * point at or past the end of buff_!
   */
  U16 offsetLast_;
};

}  // namespace omulator::msg
