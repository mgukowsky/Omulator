#include "omulator/System.hpp"

#include <sstream>

namespace omulator {

System::System(ILogger         &logger,
               std::string_view name,
               ComponentList_t  components,
               SubsystemList_t  subsystems)
  : Component(logger, name), components_(components), subsystems_(subsystems) {
  if(components_.empty()) {
    std::stringstream ss;
    ss << "System " << name << " created with no components!" << std::endl;
    logger_.warn(ss);
  }

  if(subsystems_.empty()) {
    std::stringstream ss;
    ss << "System " << name << " created with no subsystems!" << std::endl;
    logger_.warn(ss);
  }
}

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
