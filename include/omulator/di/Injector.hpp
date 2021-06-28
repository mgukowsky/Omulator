#pragma once

#include "omulator/di/TypeHash.hpp"
#include "omulator/di/TypeMap.hpp"

#include <algorithm>
#include <any>
#include <concepts>
#include <functional>
#include <map>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace omulator::di {

class Injector {
public:
  using Recipe_t    = std::function<std::any(Injector &)>;
  using RecipeMap_t = std::map<Hash_t, Recipe_t>;

  template<typename T, typename... Ts>
  void addCtorRecipe() {
    auto recipe = [](Injector &injector) { return T(injector.get<Ts>()...); };
    addRecipes({{TypeHash<T>, recipe}});
  }

  /**
   * Add recipes which should be called for specific types instead as an alternative
   * to the default injector behavior, which is to invoke the type's constructor with
   * the lowest arity.
   *
   * N.B. since this function calls std::map#merge under the hood, newRecipes will be INVALIDATED
   * after this function is invoked!;
   */
  void addRecipes(RecipeMap_t &newRecipes);

  /**
   * Support initializer lists at the call site, etc.
   */
  void addRecipes(RecipeMap_t &&newRecipes);

  /**
   * Bind an interface to an implementation. More concretely, force Injector#get<Interface>
   * to call and return Injector#get<Implementation> instead by creating a specialized recipe for
   * Interface.
   */
  template<typename RawInterface,
           typename RawImplementation,
           typename Interface      = std::remove_pointer_t<std::decay_t<RawInterface>>,
           typename Implementation = std::remove_pointer_t<std::decay_t<RawImplementation>>>
  requires std::derived_from<Implementation, Interface>
  void bindImpl() {
    auto recipe = [](Injector &injector) {
      Implementation &impl = injector.get<Implementation>();

      injector.typeMap_.emplace_impl<Interface, Implementation>(impl);

      // Return an empty std::any to as a hint to TypeMap#emplace to not instantiate an
      // instance of interface T.
      return std::any();
    };

    addRecipes({{TypeHash<Interface>, recipe}});
  }

  /**
   * Retrieve an instance of type T.
   * If type T has not yet been instantiated, then an new instance is created with the proper
   * dependencies injected. Otherwise, the instance of type T is returned.
   */
  template<typename Raw_t, typename T = std::remove_pointer_t<std::decay_t<Raw_t>>>
  T &get() {
    if(!typeMap_.has_key<T>()) {
      inject_<T>();
    }
    return typeMap_.ref<T>();
  }

private:
  template<typename T>
  void inject_() {
    auto recipeIt = std::find_if(recipeMap_.begin(), recipeMap_.end(), [this](const auto &kv) {
      return kv.first == TypeHash<T>;
    });
    /**
     * We need to wrap everything in an if constexpr block to keep compilers happy by preventing
     * them from generating code which would otherwise instantiate an abstract class.
     */
    if constexpr(std::is_abstract_v<T>) {
      // An interface can ONLY have a recipe, hence this being the only check in this block.
      if(recipeIt == recipeMap_.end()) {
        // TODO: Print out the type name in the error message (TypeString<T>?)
        throw std::runtime_error("No implementation available for abstract class; be sure to call "
                                 "Injector#bindImpl<T, Impl> before calling Injector#get<T>");
      }
      recipeMap_.at(TypeHash<T>)(*this);
    }
    else {
      if(recipeIt != recipeMap_.end()) {
        // The std::any returned by a recipe need not contain a value (e.g. in the case of an
        // interface that has an associated implementation).
        std::any anyValue = recipeIt->second(*this);
        if(anyValue.has_value()) {
          typeMap_.emplace<T>(std::any_cast<T>(anyValue));
        }
      }
      // Once again, we need if constexpr here to prevent the compiler from
      // generating code that calls emplace<T> with no arguments for types that aren't default
      // constructible.
      else if constexpr(std::default_initializable<T>) {
        typeMap_.emplace<T>();
      }
      else {
        throw std::runtime_error(
          "Could not create instance of type because it was not default initializable and there "
          "was no recipe available. Perhaps use Injector#addCtorRecipe");
      }
    }
  }

  RecipeMap_t recipeMap_;
  TypeMap     typeMap_;
};

}  // namespace omulator::di
