#include "omulator/PrimitiveIO.hpp"

#include <SDL.h>
#include <iostream>

namespace omulator::PrimitiveIO {

void log_msg(const char *msg) { std::cout << msg << std::endl; }

void log_err(const char *msg) { std::cerr << msg << std::endl; }

void alert_info(const char *msg) {
  log_msg(msg);
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Omulator: INFO", msg, nullptr);
}

void alert_err(const char *msg) {
  log_msg(msg);
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Omulator: ERROR", msg, nullptr);
}

} /* namespace omulator::PrimitiveIO */
