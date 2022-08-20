#include "omulator/System.hpp"

namespace omulator {

System::System(ILogger &logger, std::string_view name, ComponentList_t &components)
  : Component(logger, name), components_(components) { }

Cycle_t System::step(const Cycle_t numCycles) {
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
