#include "omulator/util/exception_handler.hpp"

#include "omulator/PrimitiveIO.hpp"

#include "pybind11/pytypes.h"

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
    catch([[maybe_unused]] pybind11::error_already_set &e) {
      // Errors should never escape the Interpreter class; if one does, then it's best to terminate
      // the program and not interact with the Python interpreter any further, since we have no
      // idea what thread we're on and if it will play nicely with the GIL.

      // TODO: this line (while not strictly necessary, since we're gonna terminate) makes the GCC
      // toolchain linker barf with an undefined symbol error, but only when building in release
      // mode...
      /* e.discard_as_unraisable("omulator::util::exception_handler"); */
      std::string msgText = "Unhandled Python exception!";
      omulator::PrimitiveIO::alert_err(msgText.c_str());
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
    std::abort();
  }

  /**
   * Finally, attempt proper program cleanup with a call to std::exit rather than
   * std::terminate or std::abort. N.B. that std::abort will be called anyway if something
   * goes wrong in std::exit.
   */
  std::exit(EXIT_FAILURE);
}
