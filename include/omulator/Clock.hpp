#pragma once

#include "omulator/IClock.hpp"

namespace omulator {

class Clock : public IClock {
public:
  ~Clock() override = default;

  TimePoint_t now() const noexcept override;

  virtual void sleep_until(const TimePoint_t then) override;
};

}  // namespace omulator
