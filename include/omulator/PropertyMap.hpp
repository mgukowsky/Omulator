#pragma once

#include "omulator/di/TypeHash.hpp"
#include "omulator/oml_types.hpp"
#include "omulator/util/Spinlock.hpp"

#include <atomic>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace omulator {

/**
 * Used to restrict entries in PropertyMap to a given set of types.
 */
template<typename T>
concept prop_map_type = std::same_as<S64, T> || std::same_as<U64, T> || std::
  same_as<bool, T> || std::same_as<double, T> || std::same_as<std::string, T>;

/**
 * Wrapper for a property value of a given type. The underlying value will be atomic if T is
 * trivial; otherwise access will be guarded by a Spinlock. A copy of the value can be atomically
 * retrieved using get(), and the value can be atomically set using set().
 */
template<typename Raw_t, typename T = std::decay_t<Raw_t>>
requires prop_map_type<T>
class PropertyValue {
public:
  // N.B. that this will default-initialize val_
  PropertyValue() { }
  ~PropertyValue() = default;

  // PropertyValues should only ever be created in-place within a PropertyMap
  PropertyValue(const PropertyValue &)            = delete;
  PropertyValue &operator=(const PropertyValue &) = delete;
  PropertyValue(PropertyValue &&)                 = delete;
  PropertyValue &operator=(PropertyValue &&)      = delete;

  /**
   * Retrieve a copy of val_, atomically.
   */
  T get() const noexcept {
    if constexpr(std::is_trivial_v<T>) {
      return val_.load(std::memory_order_acquire);
    }
    else {
      std::scoped_lock lck{spinlock_};
      return val_;
    }
  }

  /**
   * Set val_ atomically.
   */
  void set(const T &newVal) noexcept {
    if constexpr(std::is_trivial_v<T>) {
      val_.store(newVal, std::memory_order_release);
    }
    else {
      std::scoped_lock lck{spinlock_};
      val_ = newVal;
    }
  }

  /**
   * Overload for rvalues.
   */
  void set(T &&newVal) noexcept { set(newVal); }

private:
  /**
   * Trivial types can be used with std::atomics, while other types rely on spinlock_ for their
   * atomicity.
   */
  std::conditional_t<std::is_trivial_v<T>, std::atomic<T>, T> val_;

  // N.B. the lock is unused by trivial types
  mutable util::Spinlock spinlock_;
};

/**
 * Maps string keys to PropertyValues. Threadsafe.
 */
class PropertyMap {
public:
  /**
   * Retrieve as PropertyValue from the map. If the key has not been previously accessed, then the
   * entry for the property will be tagged with type T. If type T does not match the tag for the
   * entry (i.e. it was accessed previously as a different type), then an error will be thrown.
   *
   * Returns a reference to the underlying PropertyValue which can be used to access the property
   * from then on; it is an anti-pattern to repeatedly call get_prop in order to access a property.
   */
#if defined(OML_COMPILER_MSVC)
// The else clause is technically unreachable thanks to the constraint clause, however I'd like to
// leave it in for maintenance's sake
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
  template<typename Raw_t, typename T = std::decay_t<Raw_t>>
  requires prop_map_type<T> PropertyValue<T>
  &get_prop(std::string key) {
    std::scoped_lock lck{mtx_};

    constexpr auto typeHash = di::TypeHash<T>;

    auto entryIt = map_.find(key);
    if(entryIt == map_.end()) {
      auto ret = map_.emplace(
        std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(typeHash));
      entryIt = ret.first;
    }
    // The entry was already created; let's make sure we're interpreting it as the correct type.
    else if(entryIt->second.tag != typeHash) {
      std::stringstream ss;
      ss << "Attempted to interpret a PropertyValue as a different type from what it was "
            "previously interpreted as for key ";
      ss << key;
      throw std::runtime_error(ss.str());
    }

    auto &entry = entryIt->second;

    // We need if constexpr here to prevent the compiler from emitting code that would return a
    // PropertyValue with a different type argument than what the return value is expecting.
    if constexpr(typeHash == di::TypeHash<S64>) {
      return entry.s64Val;
    }
    if constexpr(typeHash == di::TypeHash<U64>) {
      return entry.u64Val;
    }
    else if constexpr(typeHash == di::TypeHash<bool>) {
      return entry.boolVal;
    }
    else if constexpr(typeHash == di::TypeHash<double>) {
      return entry.doubleVal;
    }
    else if constexpr(typeHash == di::TypeHash<std::string>) {
      return entry.stringVal;
    }
    else {
      std::stringstream ss;
      ss << "Invalid tag in MapEntry for key ";
      ss << key;
      throw std::runtime_error(ss.str());
    }
  }
#if defined(OML_COMPILER_MSVC)
#pragma warning(pop)
#endif

  /**
   * Returns a pair consisting of a boolean indicating if the key is present and the TypeHash of the
   * value (or 0 if the key is not present).
   */
  std::pair<bool, di::Hash_t> query_prop(std::string_view key) {
    auto it = map_.find(std::string(key));

    const bool found = (it != map_.end());
    di::Hash_t hsh   = 0;

    if(found) {
      hsh = it->second.tag;
    }

    return {found, hsh};
  }

private:
  struct MapEntry {
    MapEntry(const di::Hash_t tagArg) : tag{tagArg} {
      // We need to use placement new to properly initialize non-trivial types in the union.
      if(tag == di::TypeHash<std::string>) {
        new(&stringVal) PropertyValue<std::string>;
      }
    }

    ~MapEntry() {
      // We need to explicitly destroy non-trivial types in the union.
      if(tag == di::TypeHash<std::string>) {
        stringVal.~PropertyValue<std::string>();
      }
    }

    // std::variant doesn't play well with the atomics in PropertyValue, so we have to roll our own
    // here
    union {
      PropertyValue<S64>         s64Val;
      PropertyValue<U64>         u64Val;
      PropertyValue<bool>        boolVal;
      PropertyValue<double>      doubleVal;
      PropertyValue<std::string> stringVal;
    };

    // Store a tag for the type so that we can have some degree of type safety
    const di::Hash_t tag;
  };

  std::unordered_map<std::string, MapEntry> map_;

  std::mutex mtx_;
};

}  // namespace omulator
