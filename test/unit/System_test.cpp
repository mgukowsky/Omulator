#include "omulator/System.hpp"

#include "mocks/LoggerMock.hpp"

#include <gtest/gtest.h>

#include <functional>
#include <string_view>
#include <vector>

using omulator::Component;
using omulator::Cycle_t;
using omulator::ILogger;
using omulator::System;

namespace {

class A : public Component {
public:
  A(ILogger &logger, std::string_view name, std::vector<std::string_view> &vs)
    : Component(logger, name), numCycles_(0), vs_(vs) { }
  ~A() override = default;

  Cycle_t step(const Cycle_t numCycles) override {
    numCycles_ += numCycles;
    vs_.push_back(name());
    return numCycles;
  }

  Cycle_t numCycles() const { return numCycles_; }

private:
  Cycle_t                        numCycles_;
  std::vector<std::string_view> &vs_;
};

class B : public Component {
public:
  B(ILogger &logger, std::string_view name, std::vector<std::string_view> &vs)
    : Component(logger, name), numCycles_(0), vs_(vs) { }
  ~B() override = default;

  Cycle_t step(const Cycle_t numCycles) override {
    numCycles_ += numCycles;
    vs_.push_back(name());
    return numCycles;
  }

  Cycle_t numCycles() const { return numCycles_; }

private:
  Cycle_t                        numCycles_;
  std::vector<std::string_view> &vs_;
};

}  // namespace

TEST(System_test, simpleSystem) {
  LoggerMock                                     logger;
  std::vector<std::reference_wrapper<Component>> components;
  std::vector<std::string_view>                  recorder;

  A a(logger, "A", recorder);
  B b(logger, "B", recorder);
  components.push_back(std::ref(a));
  components.push_back(std::ref(b));

  [[maybe_unused]] System system(logger, "testsystem", components);

  EXPECT_EQ(3, system.step(3)) << "System::step should return the number of cycles taken";

  EXPECT_EQ(3, a.numCycles())
    << "System::step should call the step() function for each of its Components, in the order they "
       "are provided to the System instance";
  EXPECT_EQ(3, b.numCycles())
    << "System::step should call the step() function for each of its Components, in the order they "
       "are provided to the System instance";

  EXPECT_EQ("A", recorder.at(0))
    << "System::step should call the step() function for each of its Components, in the order they "
       "are provided to the System instance";
  EXPECT_EQ("B", recorder.at(1))
    << "System::step should call the step() function for each of its Components, in the order they "
       "are provided to the System instance";
  EXPECT_EQ("A", recorder.at(2))
    << "System::step should call the step() function for each of its Components, in the order they "
       "are provided to the System instance";
  EXPECT_EQ("B", recorder.at(3))
    << "System::step should call the step() function for each of its Components, in the order they "
       "are provided to the System instance";
  EXPECT_EQ("A", recorder.at(4))
    << "System::step should call the step() function for each of its Components, in the order they "
       "are provided to the System instance";
  EXPECT_EQ("B", recorder.at(5))
    << "System::step should call the step() function for each of its Components, in the order they "
       "are provided to the System instance";
  EXPECT_EQ(6, recorder.size())
    << "System::step should call the step() function for each of its Components, in the order they "
       "are provided to the System instance";
}
