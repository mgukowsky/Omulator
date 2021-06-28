#include "omulator/di/Injector.hpp"

namespace omulator::di {

Injector::Injector() : isInCycleCheck(false) { }

void Injector::addRecipes(RecipeMap_t &newRecipes) { recipeMap_.merge(newRecipes); }

void Injector::addRecipes(RecipeMap_t &&newRecipes) { recipeMap_.merge(newRecipes); }

}  // namespace omulator::di
