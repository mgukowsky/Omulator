#include "omulator/Component.hpp"

namespace omulator {

Component::Component(ILogger &logger, std::string_view name) : logger_{logger}, name_{name} {
  std::string str("Creating component: ");
  str += name_;
  logger_.info(str.c_str());
}

std::string_view Component::name() const noexcept { return name_; }

Cycle_t Component::step([[maybe_unused]] const Cycle_t numCycles) {
  /* no-op */
  return 0;
}

}  // namespace omulator
