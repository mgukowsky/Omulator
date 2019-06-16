#include "omulator/PrimitiveIO.hpp"
#include "omulator/logger.hpp"
#include "omulator/main.hpp"

#include <exception>
#include <stdexcept>
#include <string>

int main([[maybe_unused]] const int argc, [[maybe_unused]] const char **argv) {
  try {
    omulator::Logger l(omulator::Logger::LogLevel::TRACE);
    l.info("Hello, using spdlog!");
  }

  // Top level exception handler
  catch(...) {
    omulator::PrimitiveIO::alert_err("Something went wrong...");
    return 1;
  }

  return 0;
}
