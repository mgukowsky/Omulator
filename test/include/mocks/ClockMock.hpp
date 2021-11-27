#pragma once

#include "omulator/IClock.hpp"

class ClockMock : public omulator::IClock {
public:
  ClockMock(omulator::TimePoint_t initialTime) : now_(initialTime) { }

  omulator::TimePoint_t now() const noexcept override { return now_; }

  void set_now(omulator::TimePoint_t newNow) { now_ = newNow; }

private:
  omulator::TimePoint_t now_;
};
