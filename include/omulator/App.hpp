#pragma once

#include "omulator/util/Pimpl.hpp"

namespace omulator {
class App {
public:
  App();
  ~App();

  void run();

private:
  struct Impl_;
  util::Pimpl<Impl_> impl_;
};
}  // namespace omulator
