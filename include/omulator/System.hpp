#pragma once

#include "omulator/Component.hpp"
#include "omulator/ILogger.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string_view>

namespace omulator {

/**
 * An emulated system consisting of a list of Components, which are invoked synchronously, and a
 * collection of subsystems, each of which execute in parallel on their own threads.
 */
class System : public Component {
public:
  using ComponentList_t = std::vector<std::reference_wrapper<Component>>;

  System(ILogger &logger, std::string_view name, ComponentList_t &components);

  ~System() override = default;

  /**
   * Invoke step() for each component in components_ numCycles times. Each component will receive 1
   * as its numCycles parameter. Returns the actual number of cycles taken.
   */
  Cycle_t step(const Cycle_t numCycles) override;

private:
  /**
   * Each component in this list will have its step() method invoked during each call to
   * System::step().
   */
  ComponentList_t &components_;
};

}  // namespace omulator
