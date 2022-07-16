#pragma once

#include "omulator/Component.hpp"
#include "omulator/ILogger.hpp"

#include <atomic>
#include <memory>
#include <string_view>
#include <vector>

namespace omulator {

/**
 * An emulated system consisting of a list of Components, which are invoked synchronously, and a
 * collection of subsystems, each of which execute in parallel on their own threads.
 */
class System : public Component {
public:
  using ComponentList_t = std::vector<std::unique_ptr<Component>>;

  System(ILogger &logger, std::string_view name, ComponentList_t &components);

  ~System() override = default;

  bool can_step() const noexcept override;

  /**
   * Invoke step() for each component in components_ numCycles times. Each component will receive 1
   * as its numCycles parameter. Returns the actual number of cycles taken.
   */
  Cycle_t step(const Cycle_t numCycles) override;

private:
  /**
   * Each component in this list will have its step() method invoked during each call to
   * System::step(). Each component in this list MUST have a step() method and should have
   * can_step() return true!
   */
  ComponentList_t &components_;
};

}  // namespace omulator
