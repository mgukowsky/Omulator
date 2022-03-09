#pragma once

#include "omulator/IClock.hpp"
#include "omulator/oml_types.hpp"

#include <barrier>

namespace omulator {

class ClockMock : public omulator::IClock {
public:
  // We initialize barrier_ to the number of expected sleepers +1 so that wake_sleepers will either
  // trigger the completion of the barrier or wait there until the other threads using the mock have
  // called sleep_until
  ClockMock(omulator::TimePoint_t initialTime, const std::ptrdiff_t numSleepers = 1)
    : now_(initialTime), barrier_(numSleepers + 1, &ClockMock::null_completion_func_) { }
  ~ClockMock() override = default;

  omulator::TimePoint_t now() const noexcept override { return now_; }

  void set_now(omulator::TimePoint_t newNow) { now_ = newNow; }

  /**
   * This specialization will block until wake_sleepers() is called, regardless of the time point
   * argument that is provided.
   */
  void sleep_until([[maybe_unused]] const omulator::TimePoint_t then) override {
    barrier_.arrive_and_wait();
  }

  /**
   * Unblocks threads that have called sleep_until, however may wait synchronously until the number
   * of sleepers specified in the constructor call sleep_until.
   *
   * May be called repeatedly to wait for successive waves of sleepers.
   */
  void wake_sleepers() { barrier_.arrive_and_wait(); }

private:
  static void null_completion_func_() noexcept {};

  omulator::TimePoint_t                 now_;
  std::barrier<void (*)(void) noexcept> barrier_;
};

}  // namespace omulator
