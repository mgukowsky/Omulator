#pragma once

#include "omulator/oml_types.hpp"

namespace omulator {

class IClock {
public:
  virtual ~IClock() = default;

  virtual TimePoint_t now() const noexcept = 0;

  virtual void sleep_until(const TimePoint_t then) = 0;
};

}  // namespace omulator
