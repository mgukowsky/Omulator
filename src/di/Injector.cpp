#include "omulator/di/Injector.hpp"

#include <algorithm>

namespace omulator::di {

Injector::Injector() : isInCycleCheck_(false) { }

Injector::~Injector() {
  // Delete the elements in the type map in the reverse order they were constructed
  std::for_each(invocationList_.rbegin(), invocationList_.rend(), [this](const Hash_t &thash) {
    typeMap_.erase(thash);
  });
}

void Injector::addRecipes(RecipeMap_t &newRecipes) {
  std::scoped_lock lck(mtx_);
  recipeMap_.merge(newRecipes);
}

void Injector::addRecipes(RecipeMap_t &&newRecipes) {
  std::scoped_lock lck(mtx_);
  recipeMap_.merge(newRecipes);
}

}  // namespace omulator::di
