#pragma once

#include "omulator/IClock.hpp"
#include "omulator/oml_types.hpp"

#include <atomic>

namespace omulator {

class ClockMock : public omulator::IClock {
public:
  ClockMock(omulator::TimePoint_t initialTime) : now_(initialTime), barrier_(0), shouldBlock_(true) { }
  ~ClockMock() override = default;

  omulator::TimePoint_t now() const noexcept override { return now_; }

  void set_now(omulator::TimePoint_t newNow) { now_ = newNow; }

  /**
   * If set to false, then calls to sleep_until() will return immediately.
   */
  void set_should_block(const bool shouldBlock) {
    shouldBlock_.store(shouldBlock, std::memory_order_release);
  }

  /**
   * This specialization will block until wake_sleepers() is called, regardless of the time point
   * argument that is provided.
   */
  void sleep_until([[maybe_unused]] const omulator::TimePoint_t then) override {
    if(shouldBlock_.load(std::memory_order_acquire)) {
      U64 old = barrier_.load(std::memory_order_acquire);
      barrier_.wait(old);
    }
  }

  /**
   * Unblocks threads that have called sleep_until, however may wait synchronously until the number
   * of sleepers specified in the constructor call sleep_until.
   *
   * May be called repeatedly to wait for successive waves of sleepers.
   */
  void wake_sleepers() {
    barrier_.fetch_add(1, std::memory_order_acq_rel);
    barrier_.notify_all();
  }

private:
  omulator::TimePoint_t now_;
  std::atomic_uint64_t  barrier_;
  std::atomic_bool      shouldBlock_;
};

}  // namespace omulator
