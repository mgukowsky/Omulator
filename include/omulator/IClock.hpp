#pragma once

#include "omulator/oml_types.hpp"

namespace omulator {

class IClock {
public:
  virtual TimePoint_t now() const noexcept = 0;
};

}