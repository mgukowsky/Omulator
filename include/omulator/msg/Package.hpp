#pragma once

#include "omulator/di/TypeHash.hpp"
#include "omulator/msg/MessageBuffer.hpp"
#include "omulator/util/ObjectPool.hpp"
#include "omulator/util/reinterpret.hpp"

#include <type_traits>

namespace omulator::msg {

class Package {
public:
  /* // TODO: should only be a const/input iterator? */
  /* class iterator { */
  /* public: */
  /*   MessageBuffer::MessageHeader &operator*() const { return *pHdr_; } */
  /*   MessageBuffer::MessageHeader *operator->() { return pHdr_; } */

  /*   iterator &operator++() { */
  /*     // Need to walk the buffer here. Figure out when we're at the end b/c the MessageBuffer's
   */
  /*     // offsetLast_ will equal the pHdr_ (well, we'll have to maybe make it an offset instead to
   */
  /*     // make the comparison easier...) */
  /*     pHdr_ = (reinterpret_cast<U8 *>(pHdr_) + pHdr_->offsetNext_); */
  /*     if(pHdr_ == pBuff_->end()) { */
  /*       pBuff_ = pBuff_->next_buff(); */
  /*       if(pBuff_ != nullptr) { */
  /*         pHdr_ = pBuff_->begin(); */
  /*       } */
  /*     } */

  /*     return *this; */
  /*   } */

  /*   iterator operator++(int) { */
  /*     iterator tmp = *this; */
  /*     ++(*this); */
  /*     return tmp; */
  /*   } */

  /*   friend bool operator==(const iterator &a, const iterator &b) { return a.pHdr_ == b.pHdr_; }
   */
  /*   friend bool operator!=(const iterator &a, const iterator &b) { return a.pHdr_ != b.pHdr_; }
   */

  /* private: */
  /*   MessageBuffer *pBuff_; */
  /*   MessageHeader *pHdr_; */
  /* }; */

  using Pool_t = util::ObjectPool<MessageBuffer>;

  Package(Pool_t &pool);

  template<typename Raw_t, typename T = std::decay_t<Raw_t>>
  T &alloc_data() {
    // Fairly self-evident assert, so we can omit the explanation string.
    static_assert(sizeof(T) <= MessageBuffer::MAX_MSG_SIZE);

    static_assert(std::is_trivial_v<T>,
                  "A Package can only allocate messages which are of a trivial type, since neither "
                  "a constructor nor a destructor for said type will necessarily be invoked");

    static_assert(
      sizeof(T) > 0,
      "Package::alloc_data() is not intended only for types that have size > 0 and contain data. "
      "Consider using Package::alloc_msg for messages that do not contain any associated data.");

    return util::reinterpret<T>(alloc_(di::TypeHash32<T>, sizeof(T)));
  }

  void alloc_msg(const U32 id);

private:
  Pool_t &       pool_;
  MessageBuffer &head_;
  MessageBuffer *current_;

  void *alloc_(const U32 id, const MessageBuffer::Offset_t size);
  void *try_alloc_(const U32 id, const MessageBuffer::Offset_t size);
};

}  // namespace omulator::msg
