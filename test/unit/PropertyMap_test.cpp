#include "omulator/PropertyMap.hpp"

#include "mocks/LoggerMock.hpp"

#include <gtest/gtest.h>

using omulator::PropertyMap;
using omulator::PropertyValue;
using omulator::S64;
using omulator::U64;
using omulator::util::TypeHash;

TEST(PropertyMap_test, usageTest) {
  LoggerMock            logger;
  omulator::PropertyMap propertyMap(logger);

  PropertyValue<bool> &pvb = propertyMap.get_prop<bool>("boolKey");
  pvb.set(true);
  EXPECT_EQ(true, pvb.get()) << "PropertyValues should be able to get and set their internal value";

  PropertyValue<bool> &pvb2 = propertyMap.get_prop<bool>("boolKey");
  EXPECT_EQ(&pvb, &pvb2) << "PropertyMap::get_prop should always return a reference to the same "
                            "PropertyValue when passed the same key argument";

  PropertyValue<U64> &pvu = propertyMap.get_prop<U64>("u64key");
  pvu.set(123);
  EXPECT_EQ(123, pvu.get()) << "PropertyValues should be able to get and set their internal value";

  PropertyValue<S64> &pvs64 = propertyMap.get_prop<S64>("s64key");
  pvs64.set(456);
  EXPECT_EQ(456, pvs64.get())
    << "PropertyValues should be able to get and set their internal value";

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

TEST(PropertyMap_test, getPropVariantTest) {
  LoggerMock            logger;
  omulator::PropertyMap propertyMap(logger);

  constexpr auto SKEY1 = "stringkey1";
  constexpr auto SVAL1 = "stringval1";
  constexpr auto SKEY2 = "stringkey2";
  constexpr auto SVAL2 = "stringval2";

  auto &strVal = propertyMap.get_prop<std::string>(SKEY1);
  strVal.set(SVAL1);
  auto strVarVal = propertyMap.get_prop_variant(SKEY1);
  EXPECT_EQ(SVAL1, std::get<std::string>(strVarVal))
    << "PropertyMap::get_prop_variant should return the value corresponding to the provided key";
  strVarVal = SVAL2;
  EXPECT_NE(strVal.get(), std::get<std::string>(strVarVal))
    << "PropertyMap::get_prop_variant should return a copy of the corresponding value";

  auto unsetVarVal = propertyMap.get_prop_variant(SKEY2);
  EXPECT_EQ(PropertyMap::KEY_NOT_FOUND_STR, std::get<std::string>(unsetVarVal))
    << "PropertyMap::get_prop_variant should return a magic value when the requested key is not "
       "present in the map";
  EXPECT_FALSE(propertyMap.query_prop(SKEY2).first)
    << "PropertyMap::get_prop_variant should not create an entry in the PropertyMap for a key that "
       "is not yet present in the PropertyMap";

  // I don't feel like writing tests for the other member types of the variant; let's assume they
  // all work :)
}
