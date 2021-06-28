#pragma once

#include "omulator/di/TypeHash.hpp"
#include "omulator/di/TypeMap.hpp"

#include <algorithm>
#include <any>
#include <concepts>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace omulator::di {

class Injector {
public:
  using Recipe_t    = std::function<std::any(Injector &)>;
  using RecipeMap_t = std::map<Hash_t, Recipe_t>;

  /**
   * Ensure we strip off all pointers/qualifiers/etc. from any types used with the Injector
   * interface. N.B. this will not fully decay pointers to pointers.
   */
  template<typename T>
  using InjType_t = std::remove_pointer_t<std::decay_t<T>>;

  Injector();

  /**
   * Create and add a recipe for T which will create an instance of T by calling T(Ts...). Will
   * only accept Ts if T has a constructor that accepts references of all of the types in Ts in
   * order. Types with other constructor requirements should create a custom recipe with
   * addRecipes().
   */
  template<typename T, typename... Ts>
  void addCtorRecipe() {
    static_assert(std::constructible_from<T, std::add_lvalue_reference_t<InjType_t<Ts>>...>,
                  "Injector#addCtorRecipe<T, ...Ts> will only accept Ts if T has a constructor "
                  "that accepts the arguments (Ts&...)");
    auto recipe = [](Injector &injector) { return T(injector.get<InjType_t<Ts>>()...); };
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
           typename Interface      = InjType_t<RawInterface>,
           typename Implementation = InjType_t<RawImplementation>>
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
   * Same as Injector#get, except it returns a fresh instance rather than placing one in the type
   * map, and rejects abstract classes.
   *
   * TODO: Add an option to cache the dependencies for T to allow for a fast allocation path that
   * can bypass locks and map lookups.
   */
  template<typename T>
  T creat() {
    auto optionalVal = inject_<T>(true);
    if(!optionalVal.has_value()) {
      // TODO: print the type name
      throw std::runtime_error("Failed to creat value");
    }

    return optionalVal.value();
  }

  /**
   * Retrieve an instance of type T.
   * If type T has not yet been instantiated, then an new instance is created with the proper
   * dependencies injected. Otherwise, the instance of type T is returned.
   */
  template<typename Raw_t, typename T = InjType_t<Raw_t>>
  T &get() {
    if(!typeMap_.has_key<T>()) {
      if(isInCycleCheck_) {
        if(TypeHash<T> == cycleCheckTypeHash_) {
          // TODO: print the type
          throw std::runtime_error("Dependency cycle detected");
        }
        inject_<T>();
      }
      else {
        // Lock the mutex since we're at the top level of a dependency injection
        // TODO: we really don't need both mtx_ and isInCycleCheck_...
        std::scoped_lock lck(mtx_);
        isInCycleCheck_     = true;
        cycleCheckTypeHash_ = TypeHash<T>;
        inject_<T>();
        isInCycleCheck_ = false;
      }
    }
    // TODO: there should be no harm in executing this code in parallel without a lock, since we
    // don't make use of any functions that invalidate typeMap_'s iterators, however it might not
    // hurt to make some part of typeMap_ atomic...
    return typeMap_.ref<T>();
  }

private:
  template<typename T, typename Opt_t = std::conditional_t<std::is_abstract_v<T>, int, T>>
  std::optional<Opt_t> inject_(const bool forwardValue = false) {
    std::optional<Opt_t> retval;
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
          T val = std::any_cast<T>(anyValue);
          if(forwardValue) {
            return val;
          }
          else {
            typeMap_.emplace<T>(val);
          }
        }
      }
      // Once again, we need if constexpr here to prevent the compiler from
      // generating code that calls emplace<T> with no arguments for types that aren't default
      // constructible.
      else if constexpr(std::default_initializable<T>) {
        if(forwardValue) {
          retval = T();
        }
        else {
          typeMap_.emplace<T>();
        }
      }
      else {
        throw std::runtime_error(
          "Could not create instance of type because it was not default initializable and there "
          "was no recipe available. Perhaps use Injector#addCtorRecipe");
      }
    }

    return retval;
  }

  RecipeMap_t recipeMap_;
  TypeMap     typeMap_;

  // inject_ can lead to recursive inject_ calls, so we use these variables to check if a dependency
  // cycle is present. N.B. the use of these variables implies that calls to inject_ need to be
  // protected by a mutex.
  bool       isInCycleCheck_;
  Hash_t     cycleCheckTypeHash_;
  std::mutex mtx_;
};

}  // namespace omulator::di
