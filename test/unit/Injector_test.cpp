#include "omulator/di/Injector.hpp"

#include <gtest/gtest.h>

constexpr int MAGIC = 42;
struct Klass {
  int               x;
  inline static int numCalls = 0;
  Klass() : x(MAGIC) { ++numCalls; }
};

class InjectorTest : public ::testing::Test {
protected:
  void SetUp() override { Klass::numCalls = 0; }
};

TEST_F(InjectorTest, defaultConstructibleTypes) {
  omulator::di::Injector injector;

  auto &i1 = injector.get<int>();
  auto &i2 = injector.get<int>();

  EXPECT_EQ(&i1, &i2)
    << "The injector should only return one instance of a default constructible type";

  auto &k1 = injector.get<Klass>();
  auto &k2 = injector.get<Klass>();

  EXPECT_EQ(&k1, &k2)
    << "The injector should only return one instance of a default constructible type";

  EXPECT_EQ(1, Klass::numCalls)
    << "The injector should only create one instance of a default constructible type";

  EXPECT_EQ(MAGIC, k1.x)
    << "The injector should only correctly instantiate one instance of a default "
       "constructible type";
}

TEST_F(InjectorTest, recipeInvocation) {
  omulator::di::Injector injector;
  constexpr int          DMAGIC = MAGIC * 2;

  omulator::di::Injector::RecipeMap_t recipes{
    {omulator::di::TypeHash<Klass>, []([[maybe_unused]] omulator::di::Injector &inj) {
       Klass k;
       k.x = DMAGIC;
       return k;
     }}};

  injector.addRecipes(recipes);
  auto &k1 = injector.get<Klass>();
  auto &k2 = injector.get<Klass>();

  EXPECT_EQ(DMAGIC, k1.x) << "When a recipe for a type is present, the injector should utilize "
                             "that recipe instead of implicitly calling a constructor";

  EXPECT_EQ(1, Klass::numCalls)
    << "The injector should only create one instance of a type with a recipe";

  EXPECT_EQ(&k1, &k2) << "The injector should only return one instance of a type with a recipe";
}
