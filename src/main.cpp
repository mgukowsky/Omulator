#include "omulator/main.hpp"

#include "omulator/ILogger.hpp"
#include "omulator/di/Injector.hpp"
#include "omulator/util/exception_handler.hpp"

int main([[maybe_unused]] const int argc, [[maybe_unused]] const char **argv) {
  try {
    omulator::di::Injector injector;
    omulator::di::Injector::installDefaultRules(injector);
    auto &logger = injector.get<omulator::ILogger>();

    logger.info("Hello from refactored branch");
  }

  // Top level exception handler
  catch(...) {
    omulator::util::exception_handler();
  }

  return 0;
}
