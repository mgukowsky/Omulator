#include "omulator/di/Injector.hpp"

#include "omulator/PrimitiveIO.hpp"

#include <algorithm>
#include <sstream>
#include <string>

namespace omulator::di {

Injector::Injector() : isInCycleCheck_(false), pUpstream_(nullptr) {
  typeMap_.emplace_external_ptr<Injector>(this);

  // Special recipe to which will create a "child" Injector
  addRecipe<Injector>([&](Injector &inj) { return inj.containerize(new Injector(this)); });
}

Injector::Injector(Injector *pUpstream) : Injector() { pUpstream_ = pUpstream; }

Injector::~Injector() {
  // Delete the elements in the type map in the reverse order they were constructed
  std::for_each(invocationList_.rbegin(), invocationList_.rend(), [this](const Hash_t &thash) {
    typeMap_.erase(thash);
  });
}

void Injector::addRecipe(const Hash_t hsh, const Recipe_t recipe, std::string_view tname) {
  if(typeMap_.contains(hsh)) {
    std::stringstream ss;
    ss << "Overriding an existing recipe for " << tname;
    PrimitiveIO::log_msg(ss.str().c_str());
  }
  // TODO: it would be nice to use finer grained locking here, can we make some part of the
  // underlying map atomic?
  std::scoped_lock lck(mtx_);
  recipeMap_.insert_or_assign(hsh, recipe);
}

void Injector::addRecipes(RecipeMap_t &newRecipes) { addRecipes(std::move(newRecipes)); }

void Injector::addRecipes(RecipeMap_t &&newRecipes) {
  for(const auto &[k, v] : newRecipes) {
    addRecipe(k, v);
  }
}

bool Injector::is_root() const noexcept { return pUpstream_ == nullptr; }

}  // namespace omulator::di
