#include "omulator/System.hpp"

#include <cassert>

namespace omulator {

System::System(ILogger &logger, std::string_view name, ComponentList_t &components)
  : Component(logger, name), components_(components) { }

bool System::can_step() const noexcept { return true; }

Cycle_t System::step(const Cycle_t numCycles) {
  Cycle_t cyclesLeft = numCycles;

  while(cyclesLeft > 0) {
    for(auto &component : components_) {
      assert(component->can_step());

      component->step(1);
    }

    --cyclesLeft;
  }

  return cyclesLeft - cyclesLeft;
}

}  // namespace omulator
