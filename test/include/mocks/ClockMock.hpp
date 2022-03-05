#pragma once

#include "omulator/IClock.hpp"
#include "omulator/oml_types.hpp"

#include <atomic>

class ClockMock : public omulator::IClock {
public:
  ClockMock(omulator::TimePoint_t initialTime) : now_(initialTime) { }
  ~ClockMock() override = default;

  omulator::TimePoint_t now() const noexcept override { return now_; }

  void set_now(omulator::TimePoint_t newNow) { now_ = newNow; }

  /**
   * This specialization will block until wake_sleepers() is called, regardless of the time point
   * argument that is provided.
   */
  void sleep_until([[maybe_unused]] const omulator::TimePoint_t then) override {
    // Use std::atomic::wait removes the need to handle spurious waits explicity. Moreover, each
    // group of threads that might call sleep_until will wait until the counter increments to a
    // new value, at which point a new group of threads can begin waiting.
    const omulator::U32 oldValue = counter_;
    counter_.wait(oldValue);
  }

  void wake_sleepers() {
    ++counter_;
    counter_.notify_all();
  }

private:
  omulator::TimePoint_t      now_;
  std::atomic<omulator::U32> counter_;
};
