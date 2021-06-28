#include "omulator/di/Injector.hpp"

namespace omulator::di {

Injector::Injector() : isInCycleCheck_(false) { }

void Injector::addRecipes(RecipeMap_t &newRecipes) {
  std::scoped_lock lck(mtx_);
  recipeMap_.merge(newRecipes);
}

void Injector::addRecipes(RecipeMap_t &&newRecipes) {
  std::scoped_lock lck(mtx_);
  recipeMap_.merge(newRecipes);
}

}  // namespace omulator::di
