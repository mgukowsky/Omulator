#include "omulator/System.hpp"

#include <sstream>

namespace omulator {

System::System(ILogger &logger, std::string_view name, di::Injector &parentInjector)
  : Component(logger, name),
    pInjector_(parentInjector.creat<di::Injector>()),
    componentsCreated_(false),
    subsystemsCreated_(false) { }

di::Injector &System::get_injector() noexcept { return *pInjector_; }

Cycle_t System::step(const Cycle_t numCycles) {
  if(components_.empty()) {
    std::stringstream ss;
    ss << "System " << name() << " has no components!" << std::endl;
    logger_.warn(ss);
  }

  if(subsystems_.empty()) {
    std::stringstream ss;
    ss << "System " << name() << " has no subsystems!" << std::endl;
    logger_.warn(ss);
  }

  Cycle_t cyclesLeft = numCycles;

  while(cyclesLeft > 0) {
    for(auto &componentRef : components_) {
      Component &component = componentRef.get();
      component.step(1);
    }

    --cyclesLeft;
  }

  return numCycles - cyclesLeft;
}

}  // namespace omulator
