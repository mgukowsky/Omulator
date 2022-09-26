#pragma once

#include "omulator/Component.hpp"
#include "omulator/ILogger.hpp"
#include "omulator/Subsystem.hpp"
#include "omulator/di/Injector.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace omulator {

using ComponentList_t = std::vector<std::reference_wrapper<Component>>;
using SubsystemList_t = std::vector<std::reference_wrapper<Subsystem>>;

/**
 * An emulated system consisting of a list of Components, which are invoked synchronously, and a
 * collection of subsystems, each of which execute in parallel on their own threads.
 *
 * N.B. that the parentInjector will used to create a child injector (via a call to
 * parentInjector.creat<Injector>()). This child injector will be owned by the System, and any
 * dependencies managed by the child Injector will be destroyed along with the System as part of
 * System's destructor.
 */
class System : public Component {
public:
  System(ILogger &logger, std::string_view name, di::Injector &parentInjector);

  ~System() override = default;

  /**
   * Get a reference to the child Injector used by the System to manage dependencies.
   */
  di::Injector &get_injector() noexcept;

  /**
   * Install dependencies for the System. Each Component and Subsystem will be retrieved or created
   * via a call to pInjector_->get<T>().
   */
  template<typename... Ts>
  requires(std::derived_from<Ts, Component> &&...) void make_component_list() {
    if(componentsCreated_) {
      throw std::runtime_error("make_component_list() called on a System instance more than once");
    }

    components_ = ComponentList_t{pInjector_->get<Ts>()...};

    componentsCreated_ = true;
  }

  template<typename... Ts>
  requires(std::derived_from<Ts, Subsystem> &&...) void make_subsystem_list() {
    if(subsystemsCreated_) {
      throw std::runtime_error("make_subsystem_list() called on a System instance more than once");
    }

    subsystems_ = SubsystemList_t{pInjector_->get<Ts>()...};

    subsystemsCreated_ = true;
  }

  /**
   * Invoke step() for each component in components_ numCycles times. Each component will receive 1
   * as its numCycles parameter. Returns the actual number of cycles taken.
   */
  Cycle_t step(const Cycle_t numCycles) override;

private:
  /**
   * The Injector instances which manages dependencies for the System. A child Injector which
   * points to an upstream Injector, as set up in the constructor.
   */
  std::unique_ptr<di::Injector> pInjector_;

  /**
   * Each component in this list will have its step() method invoked during each call to
   * System::step().
   */
  ComponentList_t components_;

  /**
   * A list of Subsystems which will be spun up by the system, each executing on their own thread.
   */
  SubsystemList_t subsystems_;

  /**
   * The make_*_list() member functions should only be called on a System instance once; these flags
   * are used to track that policy.
   */
  bool componentsCreated_;
  bool subsystemsCreated_;
};

}  // namespace omulator
