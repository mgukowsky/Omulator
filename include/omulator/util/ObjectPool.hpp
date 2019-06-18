#pragma once

#include "omulator/PrimitiveIO.hpp"
#include "omulator/util/exception_handler.hpp"

#include <cassert>
#include <exception>
#include <forward_list>
#include <mutex>
#include <type_traits>
#include <vector>

namespace omulator::util {

/**
 * A generic object pool for trivial objects that do not need a constructor
 * or zero-initialized memory. Implemented internally as a singly-linked list
 * where the pointer to the next element is stored directly on the object
 * itself.
 *
 * Objects returned to the pool in a FIFO manner, which helps from a
 * cache coherency perspective.
 */
template<typename T>
class ObjectPool {
public:
  ObjectPool(const std::size_t initialSize)
    : nextExpansionSize_(initialSize), pNextFree_(nullptr), size_(0)
  {
    static_assert(std::is_trivial_v<T>, 
      "ObjectPool<T> is only valid if T is a trivial type; "
      "i.e. has no user-defined constructor");

    static_assert(sizeof(T) >= sizeof(void*),
      "ObjectPool<T> is only valid if sizeof(T) >= sizeof(void*)");

    grow_();
  }

  ~ObjectPool() = default;

  // no moving, no copying
  ObjectPool(const ObjectPool&) = delete;
  ObjectPool& operator=(const ObjectPool&) = delete;
  ObjectPool(ObjectPool&&) = delete;
  ObjectPool& operator=(ObjectPool&&) = delete;

  /**
   * Returns a reference to a T object from the pool. N.B. this is a RAW piece
   * piece of memory that will neither be initialized to zero(s) nor constructed.
   *
   * Will grow the pool by a factor of GROWTH_FACTOR_ if pool is empty.
   *
   * @return A reference to an instance of T
   */
  //TODO: protect with thread safety annotations!
  inline T* get() noexcept {
    std::scoped_lock lck(poolLock_);

    if (pNextFree_ == nullptr) {
      //Die if we get an allocation error
      //TODO: replace with Lippincott function
      try {
        grow_();
      }
      catch (...) {
        omulator::util::exception_handler();
      }
    }

    T *elementToReturn = pNextFree_;
    pNextFree_ = peek_();
    return elementToReturn;
  }

  /**
   * Return an object to the pool. This object will become what pNextFree_
   * points to; i.e. it will be immediately returned from the next call to get().
   *
   * N.B. there are no protections to check if T actually came from this pool!
   */
  //TODO: protect with thread safety annotations!
  inline void return_to_pool(T *elem) noexcept {
    std::scoped_lock lck(poolLock_);

    assert(is_element_from_pool_(elem));

    *(reinterpret_cast<T**>(elem)) = pNextFree_;
    pNextFree_ = elem;
  }

  /**
   * Returns the of the pool, in # of T elements.
   */
  std::size_t size() const noexcept {
    return size_;
  }

private:

  /**
   * Grow the pool by allocating a new vector and adding it to the
   * from of the poolMem_ forward_list.
   */
  void grow_() {
    // Allocation happens HERE.
    // N.B we push to the front to avoid an O(n) traversal
    auto &newVec = poolMem_.emplace_front(nextExpansionSize_);

    T *pNewMem = newVec.data();
    pNextFree_ = pNewMem;

    // Initialize each element to be a pointer to the element AFTER it,
    // or nullptr if it's the last one.
    for(std::size_t i = 0; i < nextExpansionSize_; ++i) {
      T **ppNextElem = reinterpret_cast<T**>(pNewMem);

      if((i + 1) < nextExpansionSize_) {
        *ppNextElem = pNewMem + 1;
      }
      else {
        *ppNextElem = nullptr;
      }

      ++pNewMem;
    }

    size_ += nextExpansionSize_;
    nextExpansionSize_ = std::size_t(size_ * GROWTH_FACTOR_);
  }

  /**
   * Return the pointer that pNextFree_ points to.
   * 
   * @return A valid pointer, or a nullptr indicating that
   *    the pool needs to grow.
   */
  inline T* peek_() const noexcept {
    return *(reinterpret_cast<T**>(pNextFree_));
  }

  /**
   * Given a T*, validate that it originated from this object pool.
   * Primarily for assertion purposes.
   */
  bool is_element_from_pool_(T *elem) const noexcept {
    bool fromThisPool = false;

    for(const auto &block : poolMem_) {
      if (elem >= block.data() && elem < (block.data() + block.size())) {
        fromThisPool = true;
        break;
      }
    }

    return fromThisPool;
  }

  std::size_t nextExpansionSize_;

  /**
   * A pointer the the next instance of T to return.
   *
   * When treated as a T*: points to the next available object in the pool
   * When treated as a T**: points to a pointer to the object AFTER the next
   *    available object in the pool. *(T**(pNextFree_)) will be a nullptr
   *    if there are no additional objects in the pool.
   */
  T *pNextFree_;

  std::mutex poolLock_;

  //TODO: since we push to the front of the list, do the following:
  //periodically, check if the first block in the list (which will be
  //the most recently allocated block), and deallocate it if it hasn't
  //been used in a while.
  std::forward_list<std::vector<T>> poolMem_;

  std::size_t size_;

  /**
   * Determines the size of the next allocation relative
   * to the current size of the pool.
   */
  static constexpr auto GROWTH_FACTOR_ = 1.5;
};

} /* namespace omulator::util */
