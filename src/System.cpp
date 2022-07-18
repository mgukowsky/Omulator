#include "omulator/System.hpp"

namespace omulator {

System::System(ILogger &logger, std::string_view name, ComponentList_t &components)
  : Component(logger, name), components_(components) { }

Cycle_t System::step(const Cycle_t numCycles) {
  Cycle_t cyclesLeft = numCycles;

  while(cyclesLeft > 0) {
    for(auto &component : components_) {
      component->step(1);
    }

    --cyclesLeft;
  }

  return cyclesLeft - cyclesLeft;
}

}  // namespace omulator
