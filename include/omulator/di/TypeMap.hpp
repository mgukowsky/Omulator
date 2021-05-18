#pragma once

#include "omulator/di/TypeHash.hpp"

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
  template<typename... Args>
  TypeContainer(Args &&...args)
    : ptr_(std::make_unique<T>(std::forward<Args>(args)...)), t_(*ptr_) { }

  ~TypeContainer() override = default;

  TypeContainer(TypeContainer &) = delete;
  TypeContainer &operator=(TypeContainer &) = delete;
  TypeContainer(TypeContainer &&)           = delete;
  TypeContainer &operator=(TypeContainer &&) = delete;

  inline T &ref() const noexcept { return t_; }

private:
  std::unique_ptr<T> ptr_;
  T &t_;
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
      map_.try_emplace(TypeHash<T>,
                       std::make_unique<TypeContainer<T>>(std::forward<Args>(args)...));
      return false;
    }
  }

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
  std::unordered_map<std::remove_const_t<decltype(TypeHash<void>)>,
                     std::unique_ptr<TypeContainerBase>>
    map_;
};
} /* namespace omulator::di */
