#pragma once

#include "omulator/util/Pimpl.hpp"

/**
 * Encapsulates the main loop of our application.
 *
 * Also note that all dependencies passed in via the constructor are essentially global
 * dependencies; they will exist prior to the creation of anything created by a System instance.
 */
namespace omulator {
class App {
public:
  App();
  ~App();

  /**
   * Enter the main loop; once this returns the program should exit.
   */
  void run();

private:
  struct Impl_;
  util::Pimpl<Impl_> impl_;
};
}  // namespace omulator
