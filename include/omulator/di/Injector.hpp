#pragma once

#include "omulator/di/TypeHash.hpp"
#include "omulator/di/TypeMap.hpp"
#include "omulator/util/TypeString.hpp"

#include <algorithm>
#include <concepts>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace omulator::di {
using util::TypeString;

/**
 * A class responsible for dependency injection. Can be used as a global service locator, but need
 * not be treated as a singleton. Types managed by the injector need not be aware of this injector,
 * as dependencies are injected via constructors.
 *
 * Instances of a given type are lazily created as they are requested with Injector#get, or
 * repeatedly created using Injector#creat. Type instances are creating using "recipes", which are
 * callbacks that return a type-erasing Container_t and receive the instance of the injector on
 * which the get() or creat() methods were invoked. While it is perfectly acceptible for a recipe to
 * return a Container_t directly, it is recommended to wrap the new instance of type T created by
 * the recipe using Injector#containerize and have the recipe return this wrapped value.
 *
 * Injector instances contain an internal table of these recipe functions which are used to
 * instantiate type instances. If a recipe for a given type T has been added via addRecipes, that
 * recipe will be invoked when an instance of type T is requested (either when creat() is called or
 * the first time get<T>() is invoked for type T).
 *
 * If a recipe is default-initializable, the injector does not need to have a recipe for it. As
 * such, if an instance of type T is requested and no recipe for T has been added, the injector will
 * attempt to default-initialize T. If this fails, the injector will throw an error indicating that
 * it has no recipe for T and that T is not default initializable.
 *
 * For types that are not default-initializable but have a simple constructor, the
 * addCtorRecipe()<T, ...Ts> method can be used to automatically add a recipe for type T which will
 * call the constructor T((Ts)...). For TDep in Ts, the injector will call get() if TDep is an
 * lvalue reference type, otherwise creat() will be called to create a new instance of (decayed)
 * TDep.
 *
 * It is also possible for the type to hold a polymorphic interface bound to a specific
 * implementation using the bindImpl() method. Once bindImpl() is invoked, subsequent calls to get()
 * will return a reference to the given interface pointing to the given implementation (N.B. that
 * creat() should not be used with an interface type if it is an abstract class).
 *
 * Lastly, get() and creat() will throw an exception if a circular dependency is detected (e.g. T
 * invokes the constructor of U, which invokes the constructor of T, etc.)
 */
class Injector {
public:
  using Container_t = std::unique_ptr<TypeContainerBase>;
  using Recipe_t    = std::function<Container_t(Injector &)>;
  using RecipeMap_t = std::map<Hash_t, Recipe_t>;

  /**
   * Ensure we strip off all pointers/qualifiers/etc. from any types used with the Injector
   * interface. N.B. this will not fully decay pointers to pointers.
   */
  template<typename T>
  using InjType_t = std::remove_pointer_t<std::decay_t<T>>;

  /**
   * The first thing that the Injector does upon construction is add itself to its typeMap_ as the
   * entry for the Injector type. This allows for recipes that have a dependency on the Injector
   * itself to return an instance of this class as that dependency. Also adds a specialized recipe
   * to return a "child" Injector (i.e. a new Injector with pUpstream_ set to the called Injector
   * instance) when creat() is invoked.
   */
  Injector();

  /**
   * Deletes all entries in the typeMap_ in the opposite order in which they were constructed.
   */
  ~Injector();

  /**
   * Create and add a recipe for T which will create an instance of T by calling T(Ts...). Will
   * only accept Ts if T has a constructor that accepts all of the types in Ts in
   * order. Types with other constructor requirements should create a custom recipe with
   * addRecipes().
   */
  template<typename Raw_t, typename... Ts>
  void addCtorRecipe() {
    using T = InjType_t<Raw_t>;
    static_assert(std::constructible_from<T, Ts...>,
                  "Injector#addCtorRecipe<T, ...Ts> will only accept Ts if T has a constructor "
                  "that accepts the arguments (Ts&...)");
    auto recipe = [](Injector &injector) {
      T *pT = new T(injector.ctorArgDispatcher_<Ts>(injector)...);
      return injector.containerize(pT);
    };
    addRecipes({
      {TypeHash<T>, recipe}
    });
  }

  /**
   * Add a recipe which should be called for a specific type instead as an alternative
   * to the default injector behavior, which is to attempt to default construct an instance of the
   * requested type.
   *
   * It is recommended that recipe callbacks make use of Injector#containerize in order to ensure
   * the correct return type.
   *
   * If a recipe for type T has been previously added, it will be overwritten by the argument given
   * to this function.
   */
  void addRecipe(const Hash_t     hsh,
                 const Recipe_t   recipe,
                 std::string_view tname = "<type name unavailable>");

  /**
   * Alternative API for addRecipe.
   */
  template<typename Raw_t, typename T = InjType_t<Raw_t>>
  void addRecipe(const Recipe_t recipe) {
    addRecipe(TypeHash<T>, recipe, TypeString<T>);
  }

  /**
   * Batched version of addRecipe.
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
      auto &impl = injector.get<Implementation>();

      injector.typeMap_.emplace_impl<Interface, Implementation>(impl);

      // Return an empty container as a hint to TypeMap#emplace to not instantiate an
      // instance of interface T.
      return Container_t(nullptr);
    };

    addRecipes({
      {TypeHash<Interface>, recipe}
    });
  }

  /**
   * Not the prettiest function in the world, but provides a nice convenience function for type
   * erasure.
   */
  template<typename T>
  Container_t containerize(T *pT) {
    std::unique_ptr<TypeContainer<T>> pCtr = std::make_unique<TypeContainer<T>>();
    pCtr->reset(pT);
    TypeContainerBase *pBaseCtr = pCtr.release();
    return Container_t(pBaseCtr);
  }

  /**
   * Same as Injector#get, except it returns a fresh instance rather than placing one in the type
   * map, and rejects abstract classes.
   *
   * Originally we considered having creat() just return a fresh instance of T, however this would
   * require that T be move-constructible, as it would need to be moved _out_ of the TypeContainer
   * used to create it via makeDependency_<T>(). Instead, it is easier to instantiate the instance
   * once within the TypeContainer and instead return a std::unique_ptr that takes ownership of it.
   * If the user really needs direct access to a raw instance of T, they can simply move the
   * instance out of the unique_ptr returned by this function.
   *
   * TODO: Add an option to cache the dependencies for T to allow for a fast allocation path that
   * can bypass locks and map lookups (and maybe use a faster allocator?).
   */
  template<typename Raw_t, typename T = InjType_t<Raw_t>>
  std::unique_ptr<T> creat() {
    static_assert(
      !std::is_abstract_v<T>,
      "An instance of an abstract type cannot be created using Injector#creat(), even if it was "
      "already bound to an implementation with bindImpl<T, implementation>, as this would require "
      "instantiating an abstract type. As an alternative, use Injector#get<T>, or create an "
      "instance of the implementation type with Injector#creat<implementation>");
    TypeContainer<T> ctr = makeDependency_<T>(true);
    if(!ctr.has_value()) {
      std::string s("Failed to create value of type ");
      s += util::TypeString<T>;
      throw std::runtime_error(s);
    }

    // No need to call std::make_unique b/c the allocation has been performed already; we're just
    // taking ownership of ctr's pointer
    return std::unique_ptr<T>(ctr.release());
  }

  /**
   * Retrieve an instance of type T.
   * If type T has not yet been instantiated, then an new instance is created with the proper
   * dependencies injected.
   *
   * If the injector is not a root injector (i.e. pUpstream_ is not null), then we will attempt to
   * retrieve the dependency from the upstream injector. If the upstream injector does not have an
   * instance of the dependency, then a new instance will be created and managed by this injector,
   * NOT the upstream injector.
   *
   * Otherwise, the instance of type T managed by this injecor is returned.
   */
  template<typename Raw_t, typename T = InjType_t<Raw_t>>
  T &get() {
    if(!has_instance<T>()) {
      if(!is_root() && pUpstream_->has_instance<T>()) {
        return pUpstream_->get<T>();
      }
      else {
        makeDependency_<T>();
      }
    }
    // TODO: there should be no harm in executing this code in parallel without a lock, since we
    // don't make use of any functions that invalidate typeMap_'s iterators, however it might not
    // hurt to make some part of typeMap_ atomic...
    return typeMap_.ref<T>();
  }

  static void installDefaultRules(Injector &injector);

  template<typename Raw_t, typename T = InjType_t<Raw_t>>
  bool has_instance() const {
    // TODO: needs (mutable) mtx?
    return typeMap_.has_key<T>();
  }

  bool is_root() const noexcept;

private:
  Injector(Injector *pUpstream);

  /**
   * Used internally by addCtorRecipe, dispatches to the appropriate behavior depending on the
   * qualifiers of Raw_t. N.B. that a new instance of T will be created if Raw_t is neither a
   * reference nor a pointer.
   */
  template<
    typename Raw_t,
    typename T = std::conditional_t<std::is_lvalue_reference_v<Raw_t>, Raw_t, InjType_t<Raw_t>>>
  std::conditional_t<std::is_pointer_v<Raw_t>, Raw_t, T> ctorArgDispatcher_(Injector &injector) {
    if constexpr(std::is_lvalue_reference_v<T>) {
      return injector.get<T>();
    }
    else if constexpr(std::is_pointer_v<Raw_t>) {
      return &(injector.get<T>());
    }
    else {
      return std::move(*(injector.creat<T>()));
    }
  }

  /**
   * Perform the actual injection.
   *
   * N.B. that Opt_t is necessary to prevent TypeContainer from receiving an abstract interface as a
   * type argument, as this would lead to compiler errors in generating code to store the abstract
   * interface in the TypeContainer instance.
   */
  template<typename T, typename Opt_t = std::conditional_t<std::is_abstract_v<T>, int, T>>
  TypeContainer<Opt_t> inject_(const bool forwardValue = false) {
    TypeContainer<Opt_t> retval;
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
        std::string s("No implementation available for abstract class ");
        s += TypeString<T>;
        s += "; be sure to call Injector#bindImpl<T, Impl> before calling Injector#get<T>";
        throw std::runtime_error(s);
      }
      recipeMap_.at(TypeHash<T>)(*this);
    }
    else {
      if(recipeIt != recipeMap_.end()) {
        // The container returned by a recipe need not contain a value (e.g. in the case of an
        // interface that has an associated implementation).
        Container_t anyValue = recipeIt->second(*this);
        if(anyValue.get() != nullptr) {
          auto *pCtr = reinterpret_cast<TypeContainer<T> *>(anyValue.get());
          T    *pVal = pCtr->release();
          if(forwardValue) {
            retval.reset(pVal);
          }
          else {
            typeMap_.emplace_ptr<T>(pVal);
          }
        }
      }

      // Once again, we need if constexpr here to prevent the compiler from
      // generating code that calls emplace<T> with no arguments for types that aren't default
      // constructible.
      else if constexpr(std::default_initializable<T>) {
        if(forwardValue) {
          retval.createInstance();
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

  /**
   * Abstracts away logic between get() and creat() that performs cycle checking and guarding access
   * to the TypeMap instance.
   */
  template<typename T, typename Opt_t = std::conditional_t<std::is_abstract_v<T>, int, T>>
  TypeContainer<Opt_t> makeDependency_(const bool forwardValue = false) {
    TypeContainer<Opt_t> retval;

    const auto thash = TypeHash<T>;
    if(isInCycleCheck_) {
      if(typeHashStack_.contains(thash)) {
        std::string s("Dependency cycle detected for type ");
        s += TypeString<T>;
        throw std::runtime_error(s);
      }
      typeHashStack_.insert(thash);
      retval = inject_<T>(forwardValue);
    }
    else {
      // Lock the mutex since we're at the top level of a dependency injection
      std::scoped_lock lck(mtx_);
      isInCycleCheck_ = true;

      // 2 line DRY violation here, but less ugly than the alternatives.
      typeHashStack_.insert(thash);
      retval          = inject_<T>(forwardValue);
      isInCycleCheck_ = false;
    }

    typeHashStack_.erase(thash);

    // If we're not calling with creat(), then we're creating the instance being placed in the type
    // map, so record when it was instantiated.
    if(!forwardValue) {
      invocationList_.push_back(thash);
    }

    return retval;
  }

  RecipeMap_t recipeMap_;
  TypeMap     typeMap_;

  // Tracks the order in which type map instances are instantiated; used to ensure that dependencies
  // are destroyed in the correct order, i.e. we destroy the dependencies in the reverse order in
  // which they are created to ensure we don't destroy a dependency before its dependent(s).
  std::vector<Hash_t> invocationList_;

  // Tracks the types that are currently being injected; used to detect cycles. Though we use this
  // as a stack, we choose a set since we'll be searching it frequently.
  std::set<Hash_t> typeHashStack_;

  // Ensure that mutations to typeMap_ and recipeMap_ are atomic.
  std::mutex mtx_;

  // Used to decide whether to lock mtx_ when calling get(); i.e. since get() may call get()
  // recursively for dependencies, we should only lock the mutex when calling get_ with the top
  // level type of the dependency chain.
  bool isInCycleCheck_;

  Injector *pUpstream_;
};

}  // namespace omulator::di
