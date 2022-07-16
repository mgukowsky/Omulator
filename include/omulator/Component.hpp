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
   * Returns true if the component has an step() method; if false then step() should NOT be invoked
   * for the given subclass.
   */
  virtual bool can_step() const noexcept = 0;

  /**
   * Returns the name of the component.
   */
  std::string_view name() const noexcept;

  /**
   * Method which should be invoked for a given number of cycles, and returns a number of cycles
   * taken; the two numbers may not necessarily be correlated.
   */
  virtual Cycle_t step(const Cycle_t numCycles) = 0;

protected:
  ILogger &logger_;

private:
  std::string_view name_;
};
}  // namespace omulator
