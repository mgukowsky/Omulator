#include "omulator/di/Injector.hpp"

#include <gtest/gtest.h>

#include <vector>

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

std::vector<int> v;
int              na    = 0;
int              nb    = 0;
int              nc    = 0;
int              nd    = 0;
bool             ready = false;

class Injector_test : public ::testing::Test {
protected:
  void SetUp() override {
    Klass::numCalls = 0;
    Base::numCalls  = 0;
    Impl::numCalls  = 0;
    v.clear();
    na    = 0;
    nb    = 0;
    nc    = 0;
    nd    = 0;
    ready = false;
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

TEST_F(Injector_test, addCtorRecipe) {
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

TEST_F(Injector_test, noRecipeForTypeWithNoDefaultConstructor) {
  omulator::di::Injector injector;

  EXPECT_THROW(injector.get<Komposite>(), std::runtime_error)
    << "An injector should throw when Injector#get is called with a type which is not default "
       "constructible, is not an interface, and does not have a recipe";
}

TEST_F(Injector_test, cycleCheck) {
  omulator::di::Injector injector;

  class CycleB;
  class CycleA {
  public:
    CycleA(CycleB &b) : b_(b) { }

  private:
    CycleB &b_;
  };

  class CycleB {
  public:
    CycleB(CycleA &a) : a_(a) { }

  private:
    CycleA &a_;
  };

  injector.addCtorRecipe<CycleA, CycleB>();
  injector.addCtorRecipe<CycleB, CycleA>();

  EXPECT_THROW(injector.get<CycleA>(), std::runtime_error)
    << "Injector#get should throw when a dependency cycle is detected";
  EXPECT_THROW(injector.get<CycleB>(), std::runtime_error)
    << "Injector#get should throw when a dependency cycle is detected";
}

TEST_F(Injector_test, creat) {
  omulator::di::Injector injector;

  [[maybe_unused]] Klass  k0 = injector.creat<Klass>();
  [[maybe_unused]] Klass &k1 = injector.get<Klass>();

  EXPECT_EQ(2, Klass::numCalls) << "Injector#creat should return a fresh instance of a given type "
                                   "and not cache it in the injector's type map";

  [[maybe_unused]] Klass k2 = injector.creat<Klass>();

  EXPECT_EQ(3, Klass::numCalls)
    << "Injector#creat should return a fresh instance of a given type each time it is called";
}

TEST_F(Injector_test, ignoreQualifiers) {
  omulator::di::Injector injector;

  [[maybe_unused]] auto &k1 = injector.get<Klass *>();
  [[maybe_unused]] auto &k2 = injector.get<Klass &>();
  [[maybe_unused]] auto &k3 = injector.get<const Klass>();
  [[maybe_unused]] auto &k4 = injector.get<Klass>();

  EXPECT_EQ(1, Klass::numCalls) << "Injector should properly ignore type qualifiers";
}

TEST_F(Injector_test, orderOfDestruction) {
  class C;
  class B;
  class A {
  public:
    A() { ++na; }
    ~A() {
      if(ready)
        v.push_back(4);
    }
  };
  class B {
  public:
    B(A &a) : a_(a) { ++nb; }
    ~B() {
      if(ready)
        v.push_back(3);
    }

  private:
    A &a_;
  };
  class C {
  public:
    C(B &b) : b_(b) { ++nc; }
    ~C() {
      if(ready)
        v.push_back(1);
    }

  private:
    B &b_;
  };
  class D {
  public:
    D() { ++nd; }
    ~D() {
      if(ready)
        v.push_back(2);
    }
  };

  {
    omulator::di::Injector injector;

    injector.addCtorRecipe<B, A>();
    injector.addCtorRecipe<C, B>();

    [[maybe_unused]] auto &b = injector.get<B>();
    [[maybe_unused]] auto &d = injector.get<D>();
    [[maybe_unused]] auto &c = injector.get<C>();

    // Prevent destructors for moved-from instances from mutating v before this point
    ready = true;
  }

  EXPECT_EQ(4, v.size())
    << "An injector should only invoke the destructor for each contained type once.";

  EXPECT_EQ(1, na)
    << "An injector should only invoke the constructor for each contained type once.";
  EXPECT_EQ(1, nb)
    << "An injector should only invoke the constructor for each contained type once.";
  EXPECT_EQ(1, nc)
    << "An injector should only invoke the constructor for each contained type once.";
  EXPECT_EQ(1, nd)
    << "An injector should only invoke the constructor for each contained type once.";

  EXPECT_EQ(1, v[0]) << "An injector should destroy the instances it contains in the reverse order "
                        "in which they were created.";
  EXPECT_EQ(2, v[1]) << "An injector should destroy the instances it contains in the reverse order "
                        "in which they were created.";
  EXPECT_EQ(3, v[2]) << "An injector should destroy the instances it contains in the reverse order "
                        "in which they were created.";
  EXPECT_EQ(4, v[3]) << "An injector should destroy the instances it contains in the reverse order "
                        "in which they were created.";
}
