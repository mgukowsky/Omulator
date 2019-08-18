#include "omulator/Latch.hpp"

#include <stdexcept>

namespace omulator {

Latch::Latch(std::ptrdiff_t value)
  : counter_(value), destructorInvoked_(false), ready_(false) {}

Latch::~Latch() {
  destructorInvoked_.store(true, std::memory_order_release);
}

void Latch::count_down(std::ptrdiff_t n) {
  // If either of these conditions are true we need to throw an exception,
  // as the behavior is undefined per the spec.
  if(n > counter_.load(std::memory_order_acquire)) {
    throw std::runtime_error("Latch::count_down called with a value that would cause the "
      "internal counter to become negative.");
  }
  else if(n < 0) {
    throw std::runtime_error("Latch::count_down called with a negative value.");
  }

  counter_.fetch_sub(n, std::memory_order_release);

  if(counter_.load(std::memory_order_acquire) == 0) {
    ready_.store(true, std::memory_order_release);
    cv_.notify_all();
  }
}

void Latch::count_down_and_wait() {
  count_down();
  wait();
}

bool Latch::is_ready() const noexcept {
  return ready_.load(std::memory_order_acquire);
}

void Latch::wait() const {
  // Per the spec
  if(counter_.load(std::memory_order_acquire) == 0) {
    return;
  }
  else if(destructorInvoked_.load(std::memory_order_acquire)) {
    throw std::runtime_error("Attempted to call Latch::wait on a Latch that is being destructed.");
  }

  std::unique_lock lck(mtx_);
  cv_.wait(lck, [this]() noexcept { return is_ready(); });
}

} /* namespace omulator */

