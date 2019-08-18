#pragma once

#include <type_traits>
#include <utility>

namespace omulator::util {

/**
 * A generic RAII wrapper; intended for managing C-style APIs with manual 
 * allocation and deallocation functions.
 * 
 * @param T the underlying type
 * @param allocator a callable which returns an instance of T.
 *    This instance will be exclusively owned by the ResourceWrapper. Does
 *    not need to be noexcept.
 * @param deallocator a callable which accepts an instance of T, by value
 *    or reference, which will be called when the ResourceWrapper falls out
 *    of scope. The deallocator should be noexcept, to avoid resource leaks.
 *    If the deallocator function needs to throw, then ResourceWrapper may not
 *    be a good fit for type T.
 */
template<typename T, auto allocator, auto deallocator>
class ResourceWrapper {
public:

  /**
   * Forwards arguments directly to the allocator function.
   */
  template<typename ...Args>
  explicit ResourceWrapper(Args &&...args)
    : instance_(allocator(std::forward<Args>(args)...))
  {

//TODO: clang-7 seems to have issues with the is_invocable checks...
#ifndef __clang__
    static_assert(std::is_invocable_r_v<T, decltype(allocator), Args...>,
      "The allocator for ResourceWrapper<T,allocator,deallocator> "
      "must return an instance of type T and be callable with the arguments "
      "provided to the constructor");
    
    static_assert(std::is_nothrow_invocable_v<decltype(deallocator), T> ||
      std::is_nothrow_invocable_v<decltype(deallocator), T&>,
      "The deallocator must be noexcept and should accept a value of type T "
      "by value or reference");
#endif
  }

  ~ResourceWrapper() {
    deallocator(instance_);
  }

private:
  T instance_;
};

} /* namespace omulator::util */
