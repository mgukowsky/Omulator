#include "omulator/util/exception_handler.hpp"

#include "omulator/PrimitiveIO.hpp"

#include <cstdlib>
#include <exception>
#include <new>
#include <string>

void omulator::util::exception_handler() noexcept {
  try {
    try {
      if(auto eptr = std::current_exception()) {
        std::rethrow_exception(eptr);
      }

      // should never happen
      else {
        omulator::PrimitiveIO::alert_err(
          "Global exception handler called without an active exception...");
      }
    }

    catch([[maybe_unused]] const std::bad_alloc &e) {
      omulator::PrimitiveIO::alert_err(
        "Memory allocation failed. This indicates that there is either not "
        "enough RAM installed on your system, or there are too many other programs "
        "running in the background.\n");
    }
    catch([[maybe_unused]] const std::exception &e) {
      std::string msgText = "An unexpected exception occurred; Details:\n";
      msgText += e.what();
      omulator::PrimitiveIO::alert_err(msgText.c_str());
    }
    catch(...) {
      omulator::PrimitiveIO::alert_err("An unknown exception occurred...");
    }
  }
  // Double fault; something is very wrong if we get here!
  catch(...) {
    omulator::PrimitiveIO::alert_err("Fault occurred in exception_handler!");
  }

  /**
   * Finally, attempt proper program cleanup with a call to std::exit rather than
   * std::terminate or std::abort. N.B. that std::abort will be called anyway if something
   * goes wrong in std::exit.
   */
  std::exit(EXIT_FAILURE);
}
