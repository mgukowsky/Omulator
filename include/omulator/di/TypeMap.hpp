#pragma once

#include "omulator/di/TypeHash.hpp"

#include <concepts>
#include <memory>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace omulator::di {
class TypeContainerBase {
public:
  virtual ~TypeContainerBase() = default;
};

template<typename T>
class TypeContainer : public TypeContainerBase {
public:
  TypeContainer() : ptr_(nullptr), hasOwnership_(false) { }

  ~TypeContainer() override { release_(); }

  TypeContainer(TypeContainer &) = delete;
  TypeContainer &operator=(TypeContainer &) = delete;
  TypeContainer(TypeContainer &&)           = delete;
  TypeContainer &operator=(TypeContainer &&) = delete;

  inline T &ref() const noexcept { return *ptr_; }

  template<typename... Args>
  void createInstance(Args &&...args) {
    ptr_          = new T(std::forward<Args>(args)...);
    hasOwnership_ = true;
  }

  void setref(T *pT) noexcept {
    release_();
    ptr_          = pT;
    hasOwnership_ = false;
  }

private:
  void release_() {
    if(hasOwnership_ && ptr_ != nullptr) {
      delete ptr_;
    }
  }

  // Use a raw pointer since we're need to manage lifetimes differently based on whether or not we
  // have ownership
  T *  ptr_;
  bool hasOwnership_;
};

class TypeMap {
public:
  /**
   * Creates an instance of type T and places it in the map. args are passed
   * to T's constructor. If type T does not yet exist in the map, an instance of
   * T is created, and this function returns false. Otherwise, this function does
   * nothing and returns true.
   */
  template<typename T, typename... Args>
  bool emplace(Args &&...args) {
    /**
     * N.B. we have to explicitly create the TypeContainer here; we can't just forward
     * the args to try_emplace by themselves, as the map would not know which type of
     * container to create.
     */

    if(has_key<T>()) {
      return true;
    }
    else {
      map_.emplace(TypeHash<T>, std::make_unique<TypeContainer<T>>());
      auto pContainerBase = map_.at(TypeHash<T>).get();
      reinterpret_cast<TypeContainer<T> *>(pContainerBase)
        ->createInstance(std::forward<Args>(args)...);
      return false;
    }
  }

  template<typename Interface, typename Impl>
  requires std::derived_from<Impl, Interface>
  bool emplace_impl(Impl &impl) {
    if(has_key<Interface>()) {
      return true;
    }
    else {
      map_.emplace(TypeHash<Interface>, std::make_unique<TypeContainer<Interface>>());
      auto pContainerBase = map_.at(TypeHash<Interface>).get();
      reinterpret_cast<TypeContainer<Interface> *>(pContainerBase)->setref(&impl);

      return false;
    }
  }

  void erase(const Hash_t hsh) { map_.erase(hsh); }

  /**
   * Clients should handle the possibility that at() can throw std::out_of_range!
   */
  template<typename T>
  inline T &ref() const {
    auto pContainerBase = map_.at(TypeHash<T>).get();
    return reinterpret_cast<TypeContainer<T> *>(pContainerBase)->ref();
  }

  template<typename T>
  inline bool has_key() const noexcept {
    // TODO: I _think_ count is slightly more efficient than find() for unordered_map in this
    // use case, since the count can only be either 0 or 1...
    return map_.count(TypeHash<T>);
  }

private:
  /**
   * We need to use a POINTER to TypeContainerBase as the value type to avoid slicing
   * issues which could arise if we stored the value directly in the map. Given this, the
   * unique_ptr here is NOT redundant with the one in TypeContainer; if we just used a raw
   * pointer as the value, then the destructor in TypeContainer would never be triggered.
   */
  std::unordered_map<Hash_t, std::unique_ptr<TypeContainerBase>> map_;
};
} /* namespace omulator::di */
