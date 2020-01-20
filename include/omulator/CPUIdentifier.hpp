#pragma once

#include "omulator/util/Pimpl.hpp"

namespace omulator {

class CPUIdentifier {
public:
  // Must be declared here for pImpl to work
  CPUIdentifier();
  ~CPUIdentifier();
  bool is_cpu_supported() const noexcept;

private:
  class CPUIdentifier_impl;
  omulator::util::Pimpl<CPUIdentifier_impl> impl_;
};

} /* namespace omulator */
