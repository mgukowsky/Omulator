#include "omulator/CPUIdentifier.hpp"
#include "omulator/Logger.hpp"
#include "omulator/main.hpp"
#include "omulator/util/exception_handler.hpp"



int main([[maybe_unused]] const int argc, [[maybe_unused]] const char **argv) {
  try {
    omulator::CPUIdentifier cpuid;
    if(!cpuid.is_cpu_supported()) {
      throw std::runtime_error("CPU is not supported");
    }
    omulator::Logger l(omulator::Logger::LogLevel::TRACE);
    l.info("Hello, using spdlog!");
  }

  // Top level exception handler
  catch(...) {
    omulator::util::exception_handler();
  }

  return 0;
}
