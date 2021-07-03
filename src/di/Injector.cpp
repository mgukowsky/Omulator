#include "omulator/di/Injector.hpp"

#include <algorithm>
#include <sstream>
#include <string>

namespace omulator::di {

Injector::Injector(ILogger &logger) : isInCycleCheck_(false), logger_(logger) { }

Injector::~Injector() {
  // Delete the elements in the type map in the reverse order they were constructed
  std::for_each(invocationList_.rbegin(), invocationList_.rend(), [this](const Hash_t &thash) {
    typeMap_.erase(thash);
  });
}

void Injector::addRecipes(RecipeMap_t &newRecipes) {
  // std::set::merge essentially moves from the container being merged, so it makes sense to just
  // move the reference here
  addRecipes(std::move(newRecipes));
}

void Injector::addRecipes(RecipeMap_t &&newRecipes) {
  for(const auto &[k, v] : newRecipes) {
    if(typeMap_.contains(k)) {
      std::stringstream ss;
      ss << "Duplicate recipe detected for type hash " << k;
      logger_.warn(ss.str().c_str());
    }
  }
  std::scoped_lock lck(mtx_);
  recipeMap_.merge(newRecipes);
}

}  // namespace omulator::di
