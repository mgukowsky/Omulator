#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/Subsystem.hpp"
#include "omulator/di/Injector.hpp"
#include "omulator/msg/Message.hpp"
#include "omulator/util/Pimpl.hpp"

#include <mutex>
#include <string>
#include <string_view>
#include <utility>

/**
 * A class which can accept string inputs which can be interpreted as script commands. Currently
 * implemented as a Python interpreter.
 */
namespace omulator {
class Interpreter : public Subsystem {
public:
  explicit Interpreter(di::Injector &injector);
  ~Interpreter() override;

  /**
   * Execute a string as a script command.
   */
  void exec(std::string_view str);

private:
  struct Impl_;
  util::Pimpl<Impl_> impl_;

  di::Injector &injector_;
  ILogger      &logger_;
};
}  // namespace omulator
