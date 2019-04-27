#pragma once

#include "omulator/util/Pimpl.hpp"

namespace omulator {

/**
 * A class encapsulating a window on the given platform.
 */
class Window {
public:
  Window();

private:
  class WindowImpl;
  omulator::util::Pimpl<WindowImpl>;
};

} /* namespace omulator */
