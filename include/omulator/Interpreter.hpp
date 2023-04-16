#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/Subsystem.hpp"
#include "omulator/di/Injector.hpp"
#include "omulator/msg/Message.hpp"

#include <mutex>
#include <string>
#include <utility>

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

private:
  di::Injector &injector_;
  ILogger      &logger_;

  /**
   * Used to enforce having only a single instance of the Interpreter at any given time.
   */
  static bool       instanceFlag_;
  static std::mutex instanceLock_;

  /**
   * Extract stdout and stdin from the interpreter, and return them as a pair of strings. Also
   * resets the io buffers used by the interpreter.
   */
  std::pair<std::string, std::string> reset_stdio_();
};
}  // namespace omulator
