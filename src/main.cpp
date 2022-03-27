#include "omulator/main.hpp"

#include "omulator/CPUIdentifier.hpp"
#include "omulator/ILogger.hpp"
#include "omulator/di/Injector.hpp"
#include "omulator/scheduler/Scheduler.hpp"
#include "omulator/util/exception_handler.hpp"

#include <chrono>

using namespace std::chrono_literals;

int main([[maybe_unused]] const int argc, [[maybe_unused]] const char **argv) {
  try {
    // TODO: This is useless, as this would have to run before any constructors that
    // run before main in order to make a difference...
    if(!omulator::CPUIdentifier::is_cpu_supported()) {
      throw std::runtime_error("CPU is not supported");
    }
    omulator::di::Injector injector;
    omulator::di::Injector::installDefaultRules(injector);
    auto &scheduler = injector.get<omulator::scheduler::Scheduler>();
    auto &logger    = injector.get<omulator::ILogger>();

    scheduler.add_job_deferred(
      [&] { logger.info("Hi!"); }, 1s, omulator::scheduler::Scheduler::SchedType::PERIODIC);

    scheduler.scheduler_main();
  }

  // Top level exception handler
  catch(...) {
    omulator::util::exception_handler();
  }

  return 0;
}
