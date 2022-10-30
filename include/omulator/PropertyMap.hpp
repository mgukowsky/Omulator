#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/oml_types.hpp"
#include "omulator/util/Spinlock.hpp"
#include "omulator/util/TypeHash.hpp"

#include <atomic>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

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
  using PropVariant_t = std::variant<S64, U64, bool, double, std::string>;

  /**
   * String value held by PropVariant_t to indicate the a given key exists in the PropertyMap but
   * has an invalid type tag.
   */
  static constexpr auto INVALID_TAG_STR = "INVALID_TAG";

  /**
   * String value held by PropVariant_t to indicate the a given key does not exist in the
   * PropertyMap.
   */
  static constexpr auto KEY_NOT_FOUND_STR = "NOT_FOUND";

  PropertyMap(ILogger &logger) : logger_(logger) { }

  /**
   * Retrieve as PropertyValue from the map. If the key has not been previously accessed, then the
   * entry for the property will be tagged with type T. If type T does not match the tag for the
   * entry (i.e. it was accessed previously as a different type), then an error will be thrown.
   *
   * Returns a reference to the underlying PropertyValue which can be used to access the property
   * from then on; it is an anti-pattern to repeatedly call get_prop in order to access a
   * property.
   *
   * Threadsafe, unlike the private version of this method.
   */
  template<typename Raw_t, typename T = std::decay_t<Raw_t>>
  requires prop_map_type<T> PropertyValue<T>
  &get_prop(std::string key) {
    std::scoped_lock lck{mtx_};

    return get_prop_<T>(key);
  }

  /**
   * Get a property value without knowing the type beforehand. Unlike get_prop, this version returns
   * a copy of the value opposed to a (PropertyValue) reference, and this version will not create an
   * entry for a property that does not yet exist.
   */
  PropVariant_t get_prop_variant(const std::string &key) {
    std::scoped_lock lck{mtx_};
    PropVariant_t    retval;

    const auto [found, hsh] = query_prop_(key);

    if(!found) {
      return KEY_NOT_FOUND_STR;
    }

    if(hsh == util::TypeHash<S64>) {
      retval = get_prop_<S64>(key).get();
    }
    else if(hsh == util::TypeHash<U64>) {
      retval = get_prop_<U64>(key).get();
    }
    else if(hsh == util::TypeHash<bool>) {
      retval = get_prop_<bool>(key).get();
    }
    else if(hsh == util::TypeHash<double>) {
      retval = get_prop_<double>(key).get();
    }
    else if(hsh == util::TypeHash<std::string>) {
      retval = get_prop_<std::string>(key).get();
    }
    else {
      retval = INVALID_TAG_STR;
    }

    return retval;
  }

  /**
   * Returns a pair consisting of a boolean indicating if the key is present and the TypeHash of the
   * value (or 0 if the key is not present).
   *
   * Threadsafe, unlike the private member version.
   */
  std::pair<bool, util::Hash_t> query_prop(std::string_view key) {
    std::scoped_lock lck{mtx_};
    return query_prop_(key);
  }

  /**
   * Set a property in the map, without knowing the type of the entry in the map. In the case that
   * the type held by the variant does not match the type of the property in the map, then an effort
   * will be made to coerce the value to the same type held by the map. If this coercion fails, then
   * the map value will be set to a default value (i.e. 0).
   *
   * If the value corresponding to the key does not yet exist in the map, then it will be created.
   * Returns true if the value in the map is successfully set, and false otherwise.
   */
  bool set_prop_variant(const std::string &prop, const PropVariant_t val) {
    std::scoped_lock lck{mtx_};
    const auto [found, hsh] = query_prop_(prop);

    if(found) {
      if(hsh == util::TypeHash<S64>) {
        get_prop_<S64>(prop).set(extract_variant_<S64>(val));
      }
      else if(hsh == util::TypeHash<U64>) {
        get_prop_<U64>(prop).set(extract_variant_<U64>(val));
      }
      else if(hsh == util::TypeHash<bool>) {
        get_prop_<bool>(prop).set(extract_variant_<bool>(val));
      }
      else if(hsh == util::TypeHash<double>) {
        get_prop_<double>(prop).set(extract_variant_<double>(val));
      }
      else if(hsh == util::TypeHash<std::string>) {
        get_prop_<std::string>(prop).set(extract_variant_<std::string>(val));
      }
      else {
        std::stringstream ss;
        ss << "Failed to set existing property '" << prop << "' due to invalid type";
        logger_.error(ss);

        return false;
      }

      return true;
    }
    else {
      // Otherwise, if the key doesn't exist, we don't have to worry about any types currently in
      // the map.
      if(std::holds_alternative<S64>(val)) {
        get_set_<S64>(prop, val);
      }
      else if(std::holds_alternative<U64>(val)) {
        get_set_<U64>(prop, val);
      }
      else if(std::holds_alternative<bool>(val)) {
        get_set_<bool>(prop, val);
      }
      else if(std::holds_alternative<double>(val)) {
        get_set_<double>(prop, val);
      }
      else if(std::holds_alternative<std::string>(val)) {
        get_set_<std::string>(prop, val);
      }
      else {
        std::stringstream ss;
        ss << "Failed to set property '" << prop << "' due to invalid type";
        logger_.error(ss);
        return false;
      }

      return true;
    }
  }

private:
  template<typename T>
  T extract_variant_(const PropVariant_t &variant) {
    // Simplest case where the variant holds the requested type
    if(std::holds_alternative<T>(variant)) {
      return std::get<T>(variant);
    }
    else if constexpr(std::is_same_v<T, std::string>) {
      return extract_variant_string_(variant);
    }
    else {
      // Other types can simply be static_cast to the requested type
      if(std::holds_alternative<S64>(variant)) {
        return static_cast<T>(std::get<S64>(variant));
      }
      else if(std::holds_alternative<U64>(variant)) {
        return static_cast<T>(std::get<U64>(variant));
      }
      else if(std::holds_alternative<bool>(variant)) {
        return static_cast<T>(std::get<bool>(variant));
      }
      else if(std::holds_alternative<double>(variant)) {
        return static_cast<T>(std::get<double>(variant));
      }
      else if(std::holds_alternative<std::string>(variant)) {
        T t{};
        // N.B. that if istringstream can't perform the conversion it will leave t as default
        // initailized.
        std::istringstream(std::get<std::string>(variant)) >> t;
        return t;
      }
      // We omit the holds_alternative<std::string> case since it should never happen
      else {
        logger_.error("Failed to extract value from PropVariant_t");
        return T{};
      }
    }
  }

  /**
   * Use a stringstream to convert a variant member to a string.
   */
  std::string extract_variant_string_(const PropVariant_t &variant) {
    std::stringstream ss;
    if(std::holds_alternative<S64>(variant)) {
      ss << std::get<S64>(variant);
    }
    else if(std::holds_alternative<U64>(variant)) {
      ss << std::get<U64>(variant);
    }
    else if(std::holds_alternative<bool>(variant)) {
      ss << std::boolalpha << std::get<bool>(variant);
    }
    else if(std::holds_alternative<double>(variant)) {
      ss << std::get<double>(variant);
    }
    else if(std::holds_alternative<std::string>(variant)) {
      ss << std::get<std::string>(variant);
    }
    else {
      logger_.error("Failed to extract std::string form PropVariant_t due to invalid type");
      ss << INVALID_TAG_STR;
    }

    return ss.str();
  }

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
  &get_prop_(std::string key) {
    constexpr auto typeHash = util::TypeHash<T>;

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
    if constexpr(typeHash == util::TypeHash<S64>) {
      return entry.s64Val;
    }
    if constexpr(typeHash == util::TypeHash<U64>) {
      return entry.u64Val;
    }
    else if constexpr(typeHash == util::TypeHash<bool>) {
      return entry.boolVal;
    }
    else if constexpr(typeHash == util::TypeHash<double>) {
      return entry.doubleVal;
    }
    else if constexpr(typeHash == util::TypeHash<std::string>) {
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

  template<typename Prop_t, typename Variant_t = Prop_t>
  void get_set_(const std::string &prop, const PropVariant_t &val) {
    get_prop_<Prop_t>(prop).set(std::get<Variant_t>(val));
  }

  /**
   * Returns a pair consisting of a boolean indicating if the key is present and the TypeHash of the
   * value (or 0 if the key is not present).
   */
  std::pair<bool, util::Hash_t> query_prop_(std::string_view key) {
    auto it = map_.find(std::string(key));

    const bool   found = (it != map_.end());
    util::Hash_t hsh   = 0;

    if(found) {
      hsh = it->second.tag;
    }

    return {found, hsh};
  }

  struct MapEntry {
    MapEntry(const util::Hash_t tagArg) : tag{tagArg} {
      // We need to use placement new to properly initialize non-trivial types in the union.
      if(tag == util::TypeHash<std::string>) {
        new(&stringVal) PropertyValue<std::string>;
      }
    }

    ~MapEntry() {
      // We need to explicitly destroy non-trivial types in the union.
      if(tag == util::TypeHash<std::string>) {
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
    const util::Hash_t tag;
  };

  ILogger &logger_;

  std::unordered_map<std::string, MapEntry> map_;

  std::mutex mtx_;
};

}  // namespace omulator
