#pragma once

#include "omulator/IClock.hpp"

namespace omulator {
 
class Clock : public IClock {
public:
  TimePoint_t now() const noexcept override;
};

}