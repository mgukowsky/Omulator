#pragma once

#include "omulator/PrimitiveIO.hpp"
#include "omulator/di/TypeMap.hpp"
#include "omulator/util/TypeHash.hpp"
#include "omulator/util/TypeString.hpp"

#include <algorithm>
#include <concepts>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace omulator::di {
using util::TypeHash;
using util::TypeString;

/**
 * A class responsible for dependency injection. Can be used as a global service locator, but need
 * not be treated as a singleton. Types managed by the injector need not be aware of this injector,
 * as dependencies are injected via constructors.
 *
 * Instances of a given type are lazily created as they are requested with Injector#get, or
 * repeatedly created using Injector#creat. Type instances are creating using "recipes", which are
 * callbacks that return a pointer to a (usually newly allocated) instance of a given type T, and
 * receive the instance of the injector on which the get() or creat() methods were invoked.
 *
 * Injector instances contain an internal table of these recipe functions which are used to
 * instantiate type instances. If a recipe for a given type T has been added via addRecipe, that
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
 * Injectors also have the ability to create a 'child' Injector: a parent ('upstream')
 * Injector will create a new Injector instance (the 'downstream') instance via a call to
 * Injector::creat<Injector>() (N.B. that a call to Injector::get<Injector>() will simply act as an
 * identity function and return a reference to the Injector which receives the call). This changes
 * the behavior of Injector::get and Injector::creat in the child Injector. If the child Injector
 * does not contain a reference to the desired type when Injector::get is called, then the call will
 * be forwarded to the parent Injector. If the parent Injector does not contain a reference to the
 * desired type, then the child Injector will follow the same logic to obtain a reference as it
 * would otherwise. If the parent Injector does have a reference to the desired type, then that
 * reference (still managed by the parent Injector!) will be returned. Injector::creat is affected
 * similarly: if the child Injector does not have a recipe for the desired type, then the parent
 * Injector will be searched for the desired recipe. N.B. that there is a special case for
 * Injector::get if neither the parent nor the child has an instance of the type BUT the parent has
 * a recipe for the type and the child does not: in such a case the parent's recipe will be used to
 * create the instance of the type, but it will be managed by the CHILD.
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
  using RecipeMap_t = std::map<util::Hash_t, Recipe_t>;

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
   * Deletes all entries in the typeMap_ in the opposite order from which they were constructed.
   */
  ~Injector();

  /**
   * Create and add a recipe for T which will create an instance of T by calling T(Ts...). Will
   * only accept Ts if T has a constructor that accepts all of the types in Ts in
   * order. Types with other constructor requirements should create a custom recipe with
   * addRecipes().
   *
   * For each TDep in Ts, the injector will call get() if TDep is an lvalue reference or pointer
   * type, otherwise creat() will be called to create a new instance of (decayed) TDep.
   *
   * Example:
   *
   *   injector.addCtorRecipe<A, B&, C, D*>()
   *
   * In this case, a recipe for A will be created which, when invoked will attempt to call a
   * constructor for A which will receive a reference to an instance of B (retrieved via
   * get()), a new instance of C as a value (initialized via creat()), and a pointer
   * to an instance of D (retrieved via get()).
   */
  template<typename Raw_t, typename... Ts>
  void addCtorRecipe() {
    using T = InjType_t<Raw_t>;
    static_assert(std::constructible_from<T, Ts...>,
                  "Injector#addCtorRecipe<T, ...Ts> will only accept Ts if T has a constructor "
                  "that accepts the arguments (Ts...)");
    auto recipe = [](Injector &injector) {
      return new T(injector.ctorArgDispatcher_<Ts>(injector)...);
    };
    addRecipe<T>(recipe);
  }

  /**
   * Add a recipe which should be called for a specific type instead as an alternative
   * to the default injector behavior, which is to attempt to default construct an instance of the
   * requested type.
   *
   * If a recipe for type T has been previously added, it will be overwritten by the argument given
   * to this function.
   */
  template<typename Raw_t, typename T = InjType_t<Raw_t>, typename RecipeFn_t>
  void addRecipe(RecipeFn_t recipe) {
    static_assert(std::is_invocable_r_v<T *, RecipeFn_t, Injector &>,
                  "A recipe function passed to addRecipe<T> must return a pointer to an instance "
                  "of T and accept a single Injector& argument");

    constexpr auto thash = TypeHash<T>;

    if(recipeMap_.contains(thash)) {
      std::stringstream ss;
      ss << "Overriding an existing recipe for " << TypeString<T>;
      PrimitiveIO::log_msg(ss.str().c_str());
    }

    Recipe_t recipeClosure = [this, recipe](Injector &) {
      return this->containerize(recipe(*this));
    };

    std::scoped_lock lck(recipeMapMtx_);

    recipeMap_.insert_or_assign(thash, recipeClosure);
  }

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

      // Return a nullptr as a hint to Injector::inject_ to not instantiate an
      // instance of interface T. We cannot return nullptr directly, as that would cause the return
      // type of this lambda to be deduced as nullptr_t (or something like that), which will cause
      // addRecipe to barf as it would have trouble translating that type into a T*
      Interface *pInterface = nullptr;
      return pInterface;
    };

    addRecipe<Interface>(recipe);
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
    TypeContainer<T> ctr = makeDependency_<T>(DepType_t::NEW_VALUE);
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
   * Version of creat() which moves the T instance out of the unique_ptr container. Requires the
   * type be moveable.
   */
  template<typename Raw_t, typename T = InjType_t<Raw_t>>
  requires std::is_move_constructible_v<T> T creat_move() { return std::move(*(creat<T>())); }

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
        makeDependency_<T>(DepType_t::REFERENCE);
      }
    }
    return typeMap_.ref<T>();
  }

  /**
   * Setup rules for the Injector by making the requisite calls to addRecipe, addCtorRecipe,
   * bindImpl, etc.
   */
  static void installDefaultRules(Injector &injector);

  /**
   * Same as installdefaultRules, except only installs the rules needed to run a subset of the
   * application's functionality before installDefaultRules is called. Should be called prior to
   * installDefaultRules in any case, as installDefaultRules may rely on rules that are first set up
   * in this function.
   */
  static void installMinimalRules(Injector &injector);

  template<typename Raw_t, typename T = InjType_t<Raw_t>>
  bool has_instance() const {
    return typeMap_.has_key<T>();
  }

  bool is_root() const noexcept;

private:
  /**
   * A tag for the type of dependency being requested.
   */
  enum class DepType_t : bool {
    /**
     * Return a reference to an existing instance of a dependency (or create one if one doesn't
     * exist).
     */
    REFERENCE,

    /**
     * Create a fresh instance of the requested dependency.
     */
    NEW_VALUE
  };

  /**
   * Specialized constructor to create a "child" Injector.
   */
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
   * Find a given recipe. If the recipe is not present in recipeMap_ and the Injector is not a root
   * Injector, then the upstream Injector will be checked for a matching recipe.
   *
   * @return A pair where the first element is a bool indicating whether or not the recipe was
   * found, and the second element is either a reference to the recipe or a reference to a null
   * recipe if no recipe was found.
   */
  std::pair<bool, Recipe_t> find_recipe_(const util::Hash_t hsh);

  /**
   * Perform the actual injection.
   *
   * N.B. that Opt_t is necessary to prevent TypeContainer from receiving an abstract interface as a
   * type argument, as this would lead to compiler errors in generating code to store the abstract
   * interface in the TypeContainer instance.
   */
  template<typename T, typename Opt_t = std::conditional_t<std::is_abstract_v<T>, int, T>>
  TypeContainer<Opt_t> inject_(const DepType_t depType) {
    TypeContainer<Opt_t> retval;
    auto [recipeFound, recipe] = find_recipe_(TypeHash<T>);

    /**
     * We need to wrap everything in an if constexpr block to keep compilers happy by preventing
     * them from generating code which would otherwise instantiate an abstract class.
     */
    if constexpr(std::is_abstract_v<T>) {
      // An interface can ONLY have a recipe, hence this being the only check in this block.
      if(!recipeFound) {
        std::string s("No implementation available for abstract class ");
        s += TypeString<T>;
        s += "; be sure to call Injector#bindImpl<T, Impl> before calling Injector#get<T>";
        throw std::runtime_error(s);
      }

      // Invoke the actual recipe, passing this Injector as an argument
      recipe(*this);
    }
    else {
      // Do we have a recipe available for the type? N.B. that this clause means that a recipe will
      // take precedence over any special cases which follow for various types.
      if(recipeFound) {
        // The container returned by a recipe need not contain a value (e.g. in the case of an
        // interface that has an associated implementation).
        Container_t anyValue = recipe(*this);
        if(anyValue.get() != nullptr) {
          auto *pCtr = reinterpret_cast<TypeContainer<T> *>(anyValue.get());
          T    *pVal = pCtr->release();
          if(depType == DepType_t::NEW_VALUE) {
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
        if(depType == DepType_t::NEW_VALUE) {
          retval.createInstance();
        }
        else {
          typeMap_.emplace<T>();
        }
      }
      else {
        std::stringstream ss;
        ss << "Could not create instance of type "
           << TypeString<T> << " because it was not default initializable and there was no recipe "
           << "available. Perhaps use Injector#addCtorRecipe";
        throw std::runtime_error(ss.str());
      }
    }

    return retval;
  }

  /**
   * Abstracts away logic between get() and creat() that performs cycle checking and guarding access
   * to the TypeMap instance.
   */
  template<typename T, typename Opt_t = std::conditional_t<std::is_abstract_v<T>, int, T>>
  TypeContainer<Opt_t> makeDependency_(const DepType_t depType) {
    TypeContainer<Opt_t> retval;

    const auto thash = TypeHash<T>;
    if(isInCycleCheck_) {
      if(typeHashStack_.contains(thash)) {
        std::string s("Dependency cycle detected for type ");
        s += TypeString<T>;
        throw std::runtime_error(s);
      }
      typeHashStack_.insert(thash);
      retval = inject_<T>(depType);
    }
    else {
      // Lock the mutex since we're at the top level of a dependency injection
      std::scoped_lock lck(injectionMtx_);
      isInCycleCheck_ = true;

      // 2 line DRY violation here, but less ugly than the alternatives.
      typeHashStack_.insert(thash);
      retval          = inject_<T>(depType);
      isInCycleCheck_ = false;
    }

    typeHashStack_.erase(thash);

    // If we're not calling with creat(), then we're creating the instance being placed in the type
    // map, so record when it was instantiated.
    if(depType != DepType_t::NEW_VALUE) {
      invocationList_.push_back(thash);
    }

    return retval;
  }

  RecipeMap_t recipeMap_;
  TypeMap     typeMap_;

  // Tracks the order in which type map instances are instantiated; used to ensure that dependencies
  // are destroyed in the correct order, i.e. we destroy the dependencies in the reverse order in
  // which they are created to ensure we don't destroy a dependency before its dependent(s).
  std::vector<util::Hash_t> invocationList_;

  // Tracks the types that are currently being injected; used to detect cycles. Though we use this
  // as a stack, we choose a set since we'll be searching it frequently.
  std::set<util::Hash_t> typeHashStack_;

  // Ensure well as injections themselves are atomic
  mutable std::mutex injectionMtx_;

  // Ensure accesses to recipeMap_ are atomic
  mutable std::mutex recipeMapMtx_;

  // Used to decide whether to lock injectionMtx_ when calling get(); i.e. since get() may call
  // get() recursively for dependencies, we should only lock the mutex when calling get_ with the
  // top level type of the dependency chain.
  bool isInCycleCheck_;

  Injector *pUpstream_;
};

}  // namespace omulator::di
