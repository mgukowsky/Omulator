#pragma once

#include "omulator/util/Pimpl.hpp"

namespace omulator {

class CPUIdentifier {
public:
  // Must be declared here for pImpl to work
  CPUIdentifier();
  ~CPUIdentifier();
  static bool is_cpu_supported() noexcept;

private:
  class CPUIdentifier_impl;
  omulator::util::Pimpl<CPUIdentifier_impl> impl_;
};

} /* namespace omulator */
