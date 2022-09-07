#include "omulator/PropertyMap.hpp"

#include <gtest/gtest.h>

using omulator::PropertyMap;
using omulator::PropertyValue;
using omulator::U64;
using omulator::di::TypeHash;

TEST(PropertyMap_test, usageTest) {
  omulator::PropertyMap propertyMap;

  PropertyValue<bool> &pvb = propertyMap.get_prop<bool>("boolKey");
  pvb.set(true);
  EXPECT_EQ(true, pvb.get()) << "PropertyValues should be able to get and set their internal value";

  PropertyValue<bool> &pvb2 = propertyMap.get_prop<bool>("boolKey");
  EXPECT_EQ(&pvb, &pvb2) << "PropertyMap::get_prop should always return a reference to the same "
                            "PropertyValue when passed the same key argument";

  PropertyValue<U64> &pvu = propertyMap.get_prop<U64>("u64key");
  pvu.set(123);
  EXPECT_EQ(123, pvu.get()) << "PropertyValues should be able to get and set their internal value";

  // I don't feel like writing the code to compare doubles; let's just assume they work :)

  PropertyValue<std::string> &pvs = propertyMap.get_prop<std::string>("stringkey");
  pvs.set("test");
  EXPECT_EQ("test", pvs.get())
    << "PropertyValues should be able to get and set their internal value";

  EXPECT_THROW(propertyMap.get_prop<bool>("stringkey"), std::runtime_error)
    << "PropertyMap::get_prop should throw when attempting to interpret a property as a different "
       "type than the type used to intialize it";

  {
    const auto [found, thash] = propertyMap.query_prop("keynotpresent");
    EXPECT_FALSE(found) << "PropertyMap::query_prop should return false for a key not in the map";
    EXPECT_EQ(0, thash)
      << "PropertyMap::query_prop should return a type hash of 0 for a key not in the map";
  }
  {
    const auto [found, thash] = propertyMap.query_prop("stringkey");
    EXPECT_TRUE(found) << "PropertyMap::query_prop should return true for a key in the map";
    EXPECT_EQ(TypeHash<std::string>, thash)
      << "PropertyMap::query_prop should return the type of the property for keys in the map";
  }
}
