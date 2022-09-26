#pragma once

#include "omulator/Component.hpp"
#include "omulator/ILogger.hpp"
#include "omulator/Subsystem.hpp"
#include "omulator/di/Injector.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string_view>

namespace omulator {

using ComponentList_t = std::vector<std::reference_wrapper<Component>>;
using SubsystemList_t = std::vector<std::reference_wrapper<Subsystem>>;

// clang-format off
template<typename... Ts>
requires (std::derived_from<Ts, Component> && ...)
auto make_component_list = [](di::Injector &injector) {
  return ComponentList_t{injector.get<Ts>()...};
};

template<typename... Ts>
requires (std::derived_from<Ts, Subsystem> && ...)
auto make_subsystem_list = [](di::Injector &injector) {
  return SubsystemList_t{injector.get<Ts>()...};
};
//clang-format on

/**
 * An emulated system consisting of a list of Components, which are invoked synchronously, and a
 * collection of subsystems, each of which execute in parallel on their own threads.
 */
class System : public Component {
public:
  System(ILogger &logger, std::string_view name, ComponentList_t components, SubsystemList_t subsystems);

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
  ComponentList_t components_;

  /**
   * A list of Subsystems which will be spun up by the system, each executing on their own thread.
   */
  SubsystemList_t subsystems_;
};

}  // namespace omulator
