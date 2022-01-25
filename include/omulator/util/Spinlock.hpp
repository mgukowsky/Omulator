#pragma once

#include "omulator/oml_defines.hpp"
#include "omulator/util/intrinsics.hpp"

#include <atomic>

namespace omulator::util {

/**
 * Classic, portable spinlock. Implements the 'Lockable' interface,
 * per the C++ standard, so that this class can be used interchangeably
 * with a mutex in many situations (i.e. with std::scoped_lock).
 */
class Spinlock {
public:
  /**
   * Default constructor. Initializes the spinlock in an unacquired state.
   * N.B. the initialization of lock_ looks awkward, but is necessary since
   * all implementations seem to define ATOMIC_FLAG_INIT with {braces} in order
   * to comply with the standard.
   */
  Spinlock() noexcept : lock_ ATOMIC_FLAG_INIT { }

  /**
   * Destructor. N.B. no attempt is made to unlock the spinlock; this
   * behavior is in line with how std::mutex and std::atomic_flag behave.
   */
  ~Spinlock() = default;

  Spinlock(const Spinlock &) = delete;
  Spinlock &operator=(const Spinlock &) = delete;
  Spinlock(Spinlock &&)                 = delete;
  Spinlock &operator=(Spinlock &&) = delete;

  /**
   * Optimistically attempt to lock the spinlock. Will block
   * until the spinlock is acquired.
   */
  OML_FORCEINLINE void lock() noexcept {
    while(!try_lock()) {
      // If available, the pause intrinsic is designed to indicate
      // to the processor that this is a spin loop.
      [[unlikely]] OML_INTRIN_PAUSE();
    }
  }

  /**
   * Attempt to lock the spinlock.
   * @return Whether or not the spinlock was acquired.
   */
  OML_FORCEINLINE bool try_lock() noexcept {
    return !(lock_.test_and_set(std::memory_order_acquire));
  }

  /**
   * Release the spinlock.
   */
  OML_FORCEINLINE void unlock() noexcept { lock_.clear(std::memory_order_release); }

private:
  /**
   * The underlying locking primitive.
   */
  std::atomic_flag lock_;

}; /* class Spinlock */

} /* namespace omulator::util */
