#include "omulator/di/Injector.hpp"

namespace omulator::di {

void Injector::addRecipes(RecipeMap_t &newRecipes) { recipeMap_.merge(newRecipes); }

}  // namespace omulator::di
