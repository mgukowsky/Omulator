#include "omulator/di/Injector.hpp"

#include <gtest/gtest.h>

constexpr int MAGIC = 42;
struct Klass {
  int               x;
  inline static int numCalls = 0;
  Klass() : x(MAGIC) { ++numCalls; }
};

class Komposite {
public:
  Komposite(Klass &k) : klass_(k) { }

private:
  Klass &klass_;
};

class Base {
public:
  Base() { ++numCalls; }
  virtual ~Base()            = default;
  inline static int numCalls = 0;
  virtual int       getnum() = 0;
};

class Impl : public Base {
public:
  Impl() { ++numCalls; }
  ~Impl() override           = default;
  inline static int numCalls = 0;
  int               getnum() override { return MAGIC; }
};

class Injector_test : public ::testing::Test {
protected:
  void SetUp() override {
    Klass::numCalls = 0;
    Base::numCalls  = 0;
    Impl::numCalls  = 0;
  }
};

TEST_F(Injector_test, defaultConstructibleTypes) {
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

TEST_F(Injector_test, recipeInvocation) {
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

TEST_F(Injector_test, interfaceAndImplementation) {
  omulator::di::Injector injector;

  // Since Base is an abstract class, this will refuse to compile if Injector attempts to
  // incorrectly instantiate Base instead of Impl.
  injector.bindImpl<Base, Impl>();

  Base &base = injector.get<Base>();
  Impl &impl = injector.get<Impl>();

  EXPECT_EQ(&base, &impl)
    << "When an injector binds an interface to an implementation, Injector#get<T> should return "
       "the same instance when T is either the interface or the implementation";

  EXPECT_EQ(1, Base::numCalls)
    << "An interface bound to an implementation should only invoke its constructor once";

  EXPECT_EQ(1, Impl::numCalls)
    << "An implementation bound to an interface should only invoke its constructor once";

  EXPECT_EQ(MAGIC, base.getnum())
    << "An interface bound to an implementation should reference the correct implementation";
}

TEST_F(Injector_test, missingInterfaceImplementation) {
  omulator::di::Injector injector;

  EXPECT_THROW(injector.get<Base>(), std::runtime_error)
    << "An injector should throw an error when an interface does not have an associated "
       "implementation";
}

TEST_F(Injector_test, extrapolatedCtorArgs) {
  omulator::di::Injector injector;

  injector.addCtorRecipe<Komposite, Klass>();

  Komposite &             komposite1 = injector.get<Komposite>();
  [[maybe_unused]] Klass &klass      = injector.get<Klass>();
  Komposite &             komposite2 = injector.get<Komposite>();

  EXPECT_EQ(1, Klass::numCalls)
    << "Injector#addCtorRecipe should add a recipe to correctly instantiate a given type";
  EXPECT_EQ(&komposite1, &komposite2)
    << "An injector should not invoke a constructor added with addCtorRecipe more than once";
}
