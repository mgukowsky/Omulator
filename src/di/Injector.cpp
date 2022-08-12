#include "omulator/di/Injector.hpp"

#include <algorithm>
#include <sstream>
#include <string>

namespace omulator::di {

Injector::Injector() : isInCycleCheck_(false), pUpstream_(nullptr) {
  typeMap_.emplace_external_ptr<Injector>(this);

  // Special recipe to which will create a "child" Injector
  addRecipe<Injector>([&]([[maybe_unused]] Injector &inj) { return new Injector(this); });
}

Injector::Injector(Injector *pUpstream) : Injector() { pUpstream_ = pUpstream; }

Injector::~Injector() {
  // Delete the elements in the type map in the reverse order they were constructed
  std::for_each(invocationList_.rbegin(), invocationList_.rend(), [this](const Hash_t &thash) {
    typeMap_.erase(thash);
  });
}

bool Injector::is_root() const noexcept { return pUpstream_ == nullptr; }

}  // namespace omulator::di
