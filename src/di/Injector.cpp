#include "omulator/di/Injector.hpp"

#include <algorithm>
#include <sstream>
#include <string>

namespace omulator::di {

namespace {
  Injector::Recipe_t nullRecipe = []([[maybe_unused]] Injector &inj) {
    int *pi = nullptr;
    return inj.containerize(pi);
  };
}  // namespace

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

std::pair<bool, Injector::Recipe_t> Injector::find_recipe_(const Hash_t hsh) {
  std::scoped_lock lck(recipeMapMtx_);

  auto recipeIt = std::find_if(
    recipeMap_.begin(), recipeMap_.end(), [this, hsh](const auto &kv) { return kv.first == hsh; });

  auto endIt = recipeMap_.end();

  if(recipeIt == endIt) {
    if(!is_root()) {
      return pUpstream_->find_recipe_(hsh);
    }
    else {
      return {false, nullRecipe};
    }
  }
  else {
    return {true, recipeIt->second};
  }
}

}  // namespace omulator::di
