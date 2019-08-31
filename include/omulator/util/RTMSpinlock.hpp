#pragma once

#include "omulator/oml_defines.hpp"
#include "omulator/util/intrinsics.hpp"

#include <atomic>

namespace omulator::util {

/**
 * A Spinlock making use of the Intel RTM extensions for even more optimistic
 * execution of locking instructions. Implements the 'BasicLockable' interface,
 * per the C++ standard, so that this class can be used interchangeably
 * with a mutex in certain situations (i.e. with std::scoped_lock).
 */
class RTMSpinlock {
public:
  /**
   * Default constructor. Initializes the spinlock in an unacquired state.
   */
  Spinlock() noexcept : lock_(false) {}
  ~Spinlock() = default;

  Spinlock(const Spinlock &) = delete;
  Spinlock &operator=(const Spinlock &) = delete;
  Spinlock(Spinlock &&) = delete;
  Spinlock &operator=(Spinlock &&) = delete;

  /**
   * Optimistically attempt to lock the spinlock. Will block
   * until the spinlock is acquired.
   */
  OML_FORCEINLINE void lock() noexcept {
    if(OML_LIKELY(_xbegin() == _XBEGIN_STARTED)) {
      if(OML_UNLIKELY(lock_.load(std::memory_order_acquire))) {
        _xabort(0xFF); //0xFF indicates that the lock could not be acquired, see Intel manual
      }
    }
    else {
      while(lock_.load(std::memory_order_acquire)) {
        _mm_pause();
      }
      lock_.exchange(true, std::memory_order_release);
    }
  }

  /**
   * Release the spinlock.
   */
  OML_FORCEINLINE void unlock() noexcept {
    if(!(lock_.load(std::memory_order_relaxed)) && _xtest()) {
      _xend();
    }
    else {
      lock_.exchange(false, std::memory_order_release);
    }
  }

private:
  /**
   * The underlying locking primitive.
   */
  std::atomic_bool lock_;

}; /* class RTMSpinlock */

} /* namespace omulator::util */
