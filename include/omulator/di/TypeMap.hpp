#pragma once

#include "omulator/di/TypeHash.hpp"

#include <concepts>
#include <memory>
#include <mutex>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace omulator::di {

// Container providing type erasure
class TypeContainerBase {
public:
  explicit TypeContainerBase(const Hash_t hsh) : hsh_(hsh) { }
  virtual ~TypeContainerBase() = default;

  /**
   * Return the TypeHash of the contained instance; used for a degree of type safety.
   */
  inline Hash_t identity() const noexcept { return hsh_; }

protected:
  Hash_t hsh_;
};

template<typename T>
class TypeContainer : public TypeContainerBase {
public:
  TypeContainer()
    : TypeContainerBase(TypeHash<std::decay_t<T>>), ptr_(nullptr), hasOwnership_(false) { }

  ~TypeContainer() override { release_(); }

  TypeContainer(TypeContainer &)            = delete;
  TypeContainer &operator=(TypeContainer &) = delete;

  // We have to define custom move operations for this type. Without this, the default move ops
  // would keep hasOwnership_ true in the moved-from instance, leading to a double-free error.
  TypeContainer(TypeContainer &&rhs) noexcept : TypeContainerBase(rhs.hsh_) {
    *this = std::move(rhs);
  }

  TypeContainer &operator=(TypeContainer &&rhs) noexcept {
    this->hsh_          = rhs.hsh_;
    this->ptr_          = rhs.ptr_;
    this->hasOwnership_ = rhs.hasOwnership_;
    rhs.hasOwnership_   = false;

    return *this;
  }

  inline T *ptr() const noexcept { return ptr_; }
  inline T &ref() const noexcept { return *ptr_; }

  // Used for two-phase initialization when a container should own a new instance of T.
  template<typename... Args>
  void createInstance(Args &&...args) {
    ptr_          = new T(std::forward<Args>(args)...);
    hasOwnership_ = true;
  }

  // std::any-esque interface to see if the type points to an instance of T.
  bool has_value() const noexcept { return ptr_ != nullptr; }

  // Relinquishes ownership of the contained instance.
  T *release() noexcept {
    T *pReleased = ptr_;

    // Prevents setref(nullptr) from deleting pReleased
    ptr_ = nullptr;
    setref(nullptr);

    return pReleased;
  }

  void reset(T *pVal) {
    setref(pVal);
    hasOwnership_ = true;
  }

  // Used for two-phase initialization when a container should have a non-owning reference to an
  // instance of T.
  void setref(T *pT) {
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
  T   *ptr_;
  bool hasOwnership_;
};

/**
 * Maps a TypeHash to an instance of the corresponding type. Completely threadsafe.
 */
class TypeMap {
public:
  bool contains(const Hash_t hsh) const noexcept {
    std::scoped_lock lck(mtx_);
    return map_.contains(hsh);
  }

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

    std::scoped_lock lck(mtx_);

    map_.emplace(TypeHash<T>, std::make_unique<TypeContainer<T>>());
    auto pContainerBase = map_.at(TypeHash<T>).get();
    reinterpret_cast<TypeContainer<T> *>(pContainerBase)
      ->createInstance(std::forward<Args>(args)...);
    return false;
  }

  template<typename Interface, typename Impl>
  requires std::derived_from<Impl, Interface>
  bool emplace_impl(Impl &impl) {
    if(has_key<Interface>()) {
      return true;
    }
    else {
      std::scoped_lock lck(mtx_);

      map_.emplace(TypeHash<Interface>, std::make_unique<TypeContainer<Interface>>());
      auto pContainerBase = map_.at(TypeHash<Interface>).get();
      reinterpret_cast<TypeContainer<Interface> *>(pContainerBase)->setref(&impl);

      return false;
    }
  }

  template<typename T>
  bool emplace_ptr(T *pT) {
    // N.B. we take ownership of pT in order to prevent a memory leak in the event that has_key<T>()
    // returns true
    std::unique_ptr<TypeContainer<T>> ctr = std::make_unique<TypeContainer<T>>();
    ctr->reset(pT);
    if(has_key<T>()) {
      return true;
    }

    std::scoped_lock lck(mtx_);
    map_.emplace(TypeHash<T>, std::move(ctr));

    return false;
  }

  /**
   * Add a entry for an unmanaged instance of a type, i.e. an instance of T external to this class
   * that is managed elsewhere.
   */
  template<typename T>
  bool emplace_external_ptr(T *pT) {
    if(has_key<T>()) {
      return true;
    }

    std::unique_ptr<TypeContainer<T>> ctr = std::make_unique<TypeContainer<T>>();

    // We don't call TypeContainer::reset() here because we DON'T want to take ownership of pT
    ctr->setref(pT);

    std::scoped_lock lck(mtx_);
    map_.emplace(TypeHash<T>, std::move(ctr));

    return false;
  }

  void erase(const Hash_t hsh) {
    std::scoped_lock lck(mtx_);
    map_.erase(hsh);
  }

  /**
   * Clients should handle the possibility that at() can throw std::out_of_range!
   */
  template<typename T>
  inline T &ref() const {
    std::scoped_lock lck(mtx_);
    auto             pContainerBase = map_.at(TypeHash<T>).get();
    return reinterpret_cast<TypeContainer<T> *>(pContainerBase)->ref();
  }

  template<typename T>
  inline bool has_key() const noexcept {
    // TODO: I _think_ count is slightly more efficient than find() for unordered_map in this
    // use case, since the count can only be either 0 or 1...
    std::scoped_lock lck(mtx_);
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

  mutable std::mutex mtx_;
};
} /* namespace omulator::di */
