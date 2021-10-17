#include "omulator/di/Injector.hpp"

#include "mocks/PrimitiveIOMock.hpp"

#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <vector>

constexpr int MAGIC  = 42;
constexpr int DMAGIC = MAGIC * 2;
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
  ~Impl() override = default;

  Impl(const Impl &) { ++numCopyCalls; }
  Impl &operator=(const Impl &) {
    ++numCopyCalls;
    return *this;
  }
  Impl(Impl &&) noexcept { ++numMoveCalls; }
  Impl &operator=(Impl &&) noexcept {
    ++numMoveCalls;
    return *this;
  }

  inline static int numCalls     = 0;
  inline static int numCopyCalls = 0;
  inline static int numMoveCalls = 0;
  int               getnum() override { return MAGIC; }
};

std::vector<int> v;
int              na    = 0;
int              nb    = 0;
int              nc    = 0;
int              nd    = 0;
bool             ready = false;

std::unique_ptr<omulator::di::Injector> pInjector;

class Injector_test : public ::testing::Test {
protected:
  void SetUp() override {
    Klass::numCalls    = 0;
    Base::numCalls     = 0;
    Impl::numCalls     = 0;
    Impl::numCopyCalls = 0;
    Impl::numMoveCalls = 0;
    v.clear();
    na    = 0;
    nb    = 0;
    nc    = 0;
    nd    = 0;
    ready = false;
    pInjector.reset(new omulator::di::Injector);
  }
};

TEST_F(Injector_test, defaultConstructibleTypes) {
  omulator::di::Injector &injector = *pInjector;

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
  omulator::di::Injector &injector = *pInjector;

  omulator::di::Injector::RecipeMap_t recipes{
    {omulator::di::TypeHash<Klass>, []([[maybe_unused]] omulator::di::Injector &inj) {
       Klass *k = new Klass;
       k->x     = DMAGIC;
       return inj.containerize(k);
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

TEST_F(Injector_test, recipeLifetime) {
  omulator::di::Injector &injector          = *pInjector;
  std::array<int, 3>      recipeInvocations = {0, 0, 0};

  omulator::di::Injector::RecipeMap_t firstRecipe{
    {omulator::di::TypeHash<Klass>, [&]([[maybe_unused]] omulator::di::Injector &inj) {
       recipeInvocations[0] += 1;
       Klass *k = new Klass;
       k->x     = 1;
       return inj.containerize(k);
     }}};
  injector.addRecipes(firstRecipe);

  omulator::di::Injector::RecipeMap_t secondRecipe{
    {omulator::di::TypeHash<Klass>, [&]([[maybe_unused]] omulator::di::Injector &inj) {
       recipeInvocations[1] += 1;
       Klass *k = new Klass;
       k->x     = 2;
       return inj.containerize(k);
     }}};
  injector.addRecipes(secondRecipe);

  auto &k1 = injector.get<Klass>();

  EXPECT_EQ(2, k1.x)
    << "When invoking Injector#get<T> for the first time (i.e. before an instance of T has been "
       "instantiated), the most recent recipe submitted to the injector should be used.";

  omulator::di::Injector::RecipeMap_t thirdRecipe{
    {omulator::di::TypeHash<Klass>, [&]([[maybe_unused]] omulator::di::Injector &inj) {
       recipeInvocations[2] += 1;
       Klass *k = new Klass;
       k->x     = 3;
       return inj.containerize(k);
     }}};
  injector.addRecipes(thirdRecipe);

  auto &k2 = injector.get<Klass>();
  EXPECT_EQ(2, k2.x)
    << "Invocations of Injector#get<T> after the first invocation should not attempt to utilize a "
       "recipe to instantiate a new instance of T, regardless of when the recipe was submitted.";

  EXPECT_EQ(0, recipeInvocations[0]) << "An injector should ignore a recipe given for a given type "
                                        "if a recipe for that type has been previously submitted";
  EXPECT_EQ(1, recipeInvocations[1]) << "An injector should ignore a recipe given for a given type "
                                        "if a recipe for that type has been previously submitted";
  EXPECT_EQ(0, recipeInvocations[2]) << "An injector should ignore a recipe given for a given type "
                                        "if a recipe for that type has been previously submitted";
}

TEST_F(Injector_test, interfaceAndImplementation) {
  omulator::di::Injector &injector = *pInjector;

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

  EXPECT_EQ(MAGIC, impl.getnum())
    << "An interface bound to an implementation should reference the correct implementation";

  [[maybe_unused]] Impl impl2 = injector.creat<Impl>();

  // N.B. The constructor for the interface will be called twice, because the move constructor for
  // the implementation (which Injector#create calls in order to return its value) will call the
  // default constructor for the interface when using Injector#creat. Since the Injector has no
  // control over this, we choose to ignore it here, but users should consider this behavior when
  // using the Injector API.
  EXPECT_EQ(3, Base::numCalls) << "When invoking Injector#creat, an interface bound to an "
                                  "implementation should only invoke its constructor once";

  EXPECT_EQ(2, Impl::numCalls) << "When invoking Injector#creat, an implementation bound to an "
                                  "interface should only invoke its constructor once";

  injector.addRecipe<Impl>([&](omulator::di::Injector &inj) {
    Impl *pImpl = new Impl;
    ++na;
    return inj.containerize(pImpl);
  });

  [[maybe_unused]] Impl impl3 = injector.creat<Impl>();

  EXPECT_EQ(1, na)
    << "When invoking Injector#creat for an implementation bound to an interface, the most recent "
       "recipe added for the implementation should be invoked";
}

TEST_F(Injector_test, missingInterfaceImplementation) {
  omulator::di::Injector &injector = *pInjector;

  EXPECT_THROW(injector.get<Base>(), std::runtime_error)
    << "An injector should throw an error when an interface does not have an associated "
       "implementation";
}

TEST_F(Injector_test, instanceConstruction) {
  omulator::di::Injector &injector = *pInjector;

  injector.bindImpl<Base, Impl>();

  [[maybe_unused]] Impl &impl1 = injector.get<Impl>();
  [[maybe_unused]] Impl &impl2 = injector.get<Impl>();

  EXPECT_EQ(1, Impl::numCalls) << "When creating an instance of a given type T using Injector#get, "
                                  "T's constructor should only be invoked once";
  EXPECT_EQ(0, Impl::numCopyCalls) << "When creating an instance of a given type T using "
                                      "Injector#get, T's copy constructor should never be called";
  EXPECT_EQ(0, Impl::numMoveCalls) << "When creating an instance of a given type T using "
                                      "Injector#get, T's move constructor should never be called";

  [[maybe_unused]] Impl impl3 = injector.creat<Impl>();
  EXPECT_EQ(2, Impl::numCalls) << "When creating an instance of a given type T using "
                                  "Injector#creat, T's constructor should only be invoked once";
  EXPECT_EQ(0, Impl::numCopyCalls) << "When creating an instance of a given type T using "
                                      "Injector#creat, T's copy constructor should never be called";
  EXPECT_EQ(1, Impl::numMoveCalls)
    << "When creating an instance of a given type T using Injector#creat, T's move constructor "
       "should only be invoked once";
}

TEST_F(Injector_test, addCtorRecipe) {
  omulator::di::Injector &injector = *pInjector;

  injector.addCtorRecipe<Komposite, Klass &>();

  Komposite &             komposite1 = injector.get<Komposite>();
  [[maybe_unused]] Klass &klass      = injector.get<Klass>();
  Komposite &             komposite2 = injector.get<Komposite>();

  EXPECT_EQ(1, Klass::numCalls)
    << "Injector#addCtorRecipe should add a recipe to correctly instantiate a given type";
  EXPECT_EQ(&komposite1, &komposite2)
    << "An injector should not invoke a constructor added with addCtorRecipe more than once";

  class A {
  public:
    A() { ++na; }
  };
  class B {
  public:
    B() { ++nb; }
  };

  class ValAValB {
  public:
    ValAValB(A a, B b) : a_(a), b_(b) { }

  private:
    A a_;
    B b_;
  };
  injector.addCtorRecipe<ValAValB, A, B>();

  class RefARefB {
  public:
    RefARefB(A &a, B &b) : a_(a), b_(b) { }

  private:
    A &a_;
    B &b_;
  };
  injector.addCtorRecipe<RefARefB, A &, B &>();

  class RefAValB {
  public:
    RefAValB(A &a, B b) : a_(a), b_(b) { }

  private:
    A &a_;
    B  b_;
  };
  injector.addCtorRecipe<RefAValB, A &, B>();

  class RefAPtrB {
  public:
    RefAPtrB(A &a, B *b) : a_(a), b_(b) { }

  private:
    A &a_;
    B *b_;
  };
  injector.addCtorRecipe<RefAPtrB, A &, B *>();

  [[maybe_unused]] ValAValB &vavb = injector.get<ValAValB>();
  EXPECT_EQ(1, na) << "An injector should call creat() to resolve dependencies when addCtorRecipe "
                      "receives those dependencies as value types.";
  EXPECT_EQ(1, nb) << "An injector should call creat() to resolve dependencies when addCtorRecipe "
                      "receives those dependencies as value types.";

  [[maybe_unused]] ValAValB &vavb2 = injector.get<ValAValB>();
  EXPECT_EQ(1, na) << "Constructor dispatching logic should not be used when get() is called for a "
                      "type T that is already in the internal type map.";
  EXPECT_EQ(1, nb) << "Constructor dispatching logic should not be used when get() is called for a "
                      "type T that is already in the internal type map.";

  [[maybe_unused]] ValAValB vavb3 = injector.creat<ValAValB>();
  EXPECT_EQ(2, na) << "An injector should call creat() to resolve dependencies when addCtorRecipe "
                      "receives those dependencies as value types.";
  EXPECT_EQ(2, nb) << "An injector should call creat() to resolve dependencies when addCtorRecipe "
                      "receives those dependencies as value types.";

  // N.B. we increment here because, since A and B were previously value types resolved with
  // creat(), they have not been added to the internal type map yet!
  [[maybe_unused]] RefARefB &rarb = injector.get<RefARefB>();
  EXPECT_EQ(3, na) << "An injector should call get() to resolve dependencies when addCtorRecipe "
                      "receives those dependencies as reference types.";
  EXPECT_EQ(3, nb) << "An injector should call get() to resolve dependencies when addCtorRecipe "
                      "receives those dependencies as reference types.";

  //...but we don't increment here since A and B are now being requested as reference types and are
  // now in the map!
  [[maybe_unused]] RefARefB rarb2 = injector.creat<RefARefB>();
  EXPECT_EQ(3, na) << "An injector should call get() to resolve dependencies when addCtorRecipe "
                      "receives those dependencies as reference types.";
  EXPECT_EQ(3, nb) << "An injector should call get() to resolve dependencies when addCtorRecipe "
                      "receives those dependencies as reference types.";

  [[maybe_unused]] RefAValB &ravb = injector.get<RefAValB>();
  EXPECT_EQ(3, na) << "An injector should correctly choose whether to call get() or creat() for a "
                      "given dependency when dependencies are given with addCtorRecipe.";
  EXPECT_EQ(4, nb) << "An injector should correctly choose whether to call get() or creat() for a "
                      "given dependency when dependencies are given with addCtorRecipe.";

  [[maybe_unused]] RefAValB ravb2 = injector.creat<RefAValB>();
  EXPECT_EQ(3, na) << "An injector should correctly choose whether to call get() or creat() for a "
                      "given dependency when dependencies are given with addCtorRecipe.";
  EXPECT_EQ(5, nb) << "An injector should correctly choose whether to call get() or creat() for a "
                      "given dependency when dependencies are given with addCtorRecipe.";

  // Pointer types of constructor arguments should be handled like reference types
  [[maybe_unused]] RefAPtrB rapb = injector.creat<RefAPtrB>();
  EXPECT_EQ(3, na) << "An injector should correctly choose whether to call get() or creat() for a "
                      "given dependency when dependencies are given with addCtorRecipe.";
  EXPECT_EQ(5, nb) << "An injector should correctly choose whether to call get() or creat() for a "
                      "given dependency when dependencies are given with addCtorRecipe.";
}

TEST_F(Injector_test, noRecipeForTypeWithNoDefaultConstructor) {
  omulator::di::Injector &injector = *pInjector;

  EXPECT_THROW(injector.get<Komposite>(), std::runtime_error)
    << "An injector should throw when Injector#get is called with a type which is not default "
       "constructible, is not an interface, and does not have a recipe";
}

TEST_F(Injector_test, cycleCheck) {
  omulator::di::Injector &injector = *pInjector;

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

  class CycleC {
  public:
    CycleC(CycleB &b) : b_(b) { }

  private:
    CycleB &b_;
  };

  injector.addCtorRecipe<CycleA, CycleB &>();
  injector.addCtorRecipe<CycleB, CycleA &>();
  injector.addCtorRecipe<CycleC, CycleB &>();

  EXPECT_THROW(injector.get<CycleA>(), std::runtime_error)
    << "Injector#get should throw when a dependency cycle is detected";
  EXPECT_THROW(injector.get<CycleB>(), std::runtime_error)
    << "Injector#get should throw when a dependency cycle is detected";
  EXPECT_THROW(injector.get<CycleC>(), std::runtime_error)
    << "Injector#get should throw when a dependency cycle is detected";
}

TEST_F(Injector_test, creat) {
  omulator::di::Injector &injector = *pInjector;

  [[maybe_unused]] Klass  k0 = injector.creat<Klass>();
  [[maybe_unused]] Klass &k1 = injector.get<Klass>();

  EXPECT_EQ(2, Klass::numCalls) << "Injector#creat should return a fresh instance of a given type "
                                   "and not cache it in the injector's type map";

  [[maybe_unused]] Klass k2 = injector.creat<Klass>();

  EXPECT_EQ(3, Klass::numCalls)
    << "Injector#creat should return a fresh instance of a given type each time it is called";

  omulator::di::Injector::RecipeMap_t recipes{
    {omulator::di::TypeHash<Klass>, []([[maybe_unused]] omulator::di::Injector &inj) {
       Klass *k = new Klass;
       k->x     = DMAGIC;
       return inj.containerize(k);
     }}};
  injector.addRecipes(recipes);

  Klass k3 = injector.creat<Klass>();

  omulator::di::Injector::RecipeMap_t moreRecipes{
    {omulator::di::TypeHash<Klass>, []([[maybe_unused]] omulator::di::Injector &inj) {
       Klass *k = new Klass;
       k->x     = DMAGIC * 2;
       return inj.containerize(k);
     }}};
  injector.addRecipes(moreRecipes);

  Klass k4 = injector.creat<Klass>();

  EXPECT_EQ(DMAGIC, k3.x) << "creat<T> should invoke the recipe for T if one is present, even if "
                             "no recipe was present during previous calls to creat<T>";
  EXPECT_EQ(DMAGIC * 2, k4.x) << "creat<T> should invoke the most recent recipe which has been "
                                 "added to the Injector instance";
}

TEST_F(Injector_test, ignoreQualifiers) {
  omulator::di::Injector &injector = *pInjector;

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

    injector.addCtorRecipe<B, A &>();
    injector.addCtorRecipe<C, B &>();

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

TEST_F(Injector_test, dependencyOnSelf) {
  omulator::di::Injector &injector = *pInjector;

  struct A {
    A(omulator::di::Injector &inj) : injector(inj) { }

    omulator::di::Injector &injector;
  };

  injector.addCtorRecipe<A, omulator::di::Injector &>();

  A a = injector.creat<A>();

  EXPECT_EQ(&injector, &(a.injector))
    << "An class that has a dependency on an Injector should receive the instance of the managing "
       "Injector as a dependency";
}
