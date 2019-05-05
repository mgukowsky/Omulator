#include "omulator/PrimitiveIO.hpp"

#include <iostream>
#include <Windows.h>

namespace omulator::PrimitiveIO {

void log_msg(const char *msg) {
  std::cout << msg << std::endl;
}

void alert_info(const char *msg) {
  log_msg(msg);
  MessageBox(nullptr, msg, "Omulator: MESSAGE", MB_OK | MB_ICONINFORMATION);
}

void alert_err(const char *msg) {
  log_msg(msg);
  MessageBox(nullptr, msg, "Omulator: ERROR", MB_OK | MB_ICONERROR);
}

} /* namespace omulator::PrimitiveIO */
