#pragma once

#include "omulator/IClock.hpp"

namespace omulator {

class Clock : public IClock {
public:
  ~Clock() override = default;

  TimePoint_t now() const noexcept override;
};

}  // namespace omulator
