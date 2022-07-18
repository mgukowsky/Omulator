#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/oml_types.hpp"

#include <string_view>

namespace omulator {

/**
 * An abstract component of an emulated system.
 */
class Component {
public:
  Component(ILogger &logger, std::string_view name);

  virtual ~Component() = default;

  /**
   * Returns the name of the component.
   */
  std::string_view name() const noexcept;

  /**
   * Method which should be invoked for a given number of cycles, and returns a number of cycles
   * taken; the two numbers may not necessarily be correlated.
   *
   * The default implementation is a no-op
   */
  virtual Cycle_t step(const Cycle_t numCycles);

protected:
  ILogger &logger_;

private:
  std::string_view name_;
};
}  // namespace omulator
