#pragma once

#include <iostream>

namespace omulator::PrimitiveIO {
  const char *lastMsgLogged = nullptr;

  void log_msg(const char *msg) { std::cout << msg; }

  void log_err(const char *msg) { std::cout << msg; std::cerr << msg; }

  void alert_info(const char *msg) { log_msg(msg); }

  void alert_err(const char *msg) { log_err(msg); }

} /* namespace omulator::PrimitiveIO */
