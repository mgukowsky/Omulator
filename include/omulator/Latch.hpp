#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace omulator {

/**
 * An implementation of std::experimental::latch.
 */
class Latch {
public:
  explicit Latch(std::ptrdiff_t value);
  ~Latch();

  // Not copyable, per the spec.
  Latch(const Latch &) = delete;
  Latch &operator=(const Latch &) = delete;

  Latch(Latch &&) = delete;
  Latch &operator=(Latch &&) = delete;

  void count_down(std::ptrdiff_t n = 1);
  void count_down_and_wait();
  bool is_ready() const noexcept;

  // const per the spec
  void wait() const;

private:
  std::ptrdiff_t counter_;

  // This variable and the associated mutex must be mutable, since Latch::wait is const
  // but these variables are used in that function.
  mutable std::condition_variable cv_;

  /**
   * Used to protect accesses to counter_
   */
  mutable std::mutex counterMtx_;

  std::atomic_bool destructorInvoked_;

  std::atomic_bool ready_;

}; /* class Latch */

// In case we ever want to switch over to the C++20 implementation...
using Latch_ty = Latch;

} /* namespace omulator */
