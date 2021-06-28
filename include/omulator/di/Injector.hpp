#pragma once

#include "omulator/di/TypeHash.hpp"
#include "omulator/di/TypeMap.hpp"

#include <algorithm>
#include <any>
#include <concepts>
#include <functional>
#include <map>
#include <type_traits>
#include <utility>

namespace omulator::di {

class Injector {
public:
  using RecipeMap_t = std::map<Hash_t, std::function<std::any(Injector &)>>;

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

      // N.B. passing an Implementation& to emplace<Interface> will trigger a specialized
      // TypeContainer constructor.
      injector.typeMap_.emplace_impl<Interface, Implementation>(impl);

      // Return an empty std::any to prevent TypeMap#emplace from instantiating an
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
     * them from generating code which would otherwise create an abstract class.
     */
    if constexpr(std::is_abstract_v<T>) {
      if(recipeIt == recipeMap_.end()) {
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
      else if(std::default_initializable<T>) {
        typeMap_.emplace<T>();
      }
      // if constexpr(<USE A CONCEPT TO FIGURE OUT IF THERE IS A RECIPE>) {
      // }
      /**
       * i.e. the type has no dependencies, and we have no recipe for it.
       */
      // else if constexpr(requires std::default_initializable<T>) {
      // typeMap_.emplace<T>();
      //}
      /**
       * Otherwise, attempt to resolve each of the dependencies we find in T's constructor,
       * throwing an error if we find one we can't resolve. We attempt to guess the dependencies
       * by finding the constructor with the lowest arity. N.B. the <TODO: what's the name?>
       * mechanism will blow up if T has two constructors with the same arity which are also T's
       * constructors with the lowest arity.
       */
      // else {
      //  inject_deps_<T>(__CTOR_MAGIC_TUPLE__);
      // }
    }
  }

  /**
   * Wires up all dependencies for type T, then creates an instance of type T.
   */
  // template<typename T, typename... Deps_ts>
  // void inject_deps_([[maybe_unused]] const std::tuple<Deps_ts...> &) {
  /**
   * N.B. the pack expansion here allows us to dig down into the dependency
   * "tree", and eventually pass the args to emplace<T>().
   */
  // typeMap_.emplace<T>((get<Deps_ts>())...);
  // }

  RecipeMap_t recipeMap_;
  TypeMap     typeMap_;
};

}  // namespace omulator::di
