#pragma once

#include "omulator/di/TypeHash.hpp"
#include "omulator/di/TypeMap.hpp"

#include <any>
#include <concepts>
#include <map>

namespace omulator::di {

class Injector {
public:
  /**
   * Retrieve an instance of type T.
   * If type T has not yet been instantiated, then an new instance is created with the proper
   * dependencies injected. Otherwise, the instance of type T is returned.
   *
   * TODO: decay the type!
   */
  template<typename Raw_t, typename T = std::remove_pointer_t<std::decay_t<Raw_t>>>
  T &get() {
    if(!typeMap_.has_key<T>()) {
      // TODO: this is dumb. Just return the fresh instance here.
      inject_<T>();
    }
    return typeMap_.ref<T>();
  }

private:
  template<typename T>
  void inject_() {
    if constexpr(std::default_initializable<T>) {
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

  // using InitializerFunc_t = std::any (*)(Injector &);
  // std::map<Hash_t, InitializerFunc_t>,
  TypeMap typeMap_;
};

}  // namespace omulator::di
