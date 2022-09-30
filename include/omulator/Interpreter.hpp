#pragma once

#include "omulator/Subsystem.hpp"
#include "omulator/di/Injector.hpp"
#include "omulator/msg/Message.hpp"

#include <string>

/**
 * A class which can accept string inputs which can be interpreted as script commands. Currently
 * implemented as a Python interpreter.
 */
namespace omulator {
class Interpreter : public Subsystem {
public:
  explicit Interpreter(di::Injector &injector);
  ~Interpreter() = default;

  /**
   * Execute a string as a script command.
   */
  void exec(std::string str);
  void message_proc(const msg::Message &msg) override;

private:
  di::Injector &injector_;
};
}  // namespace omulator