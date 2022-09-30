#include "omulator/Interpreter.hpp"

#include "omulator/ILogger.hpp"
#include "omulator/di/TypeHash.hpp"
#include "omulator/msg/MailboxRouter.hpp"
#include "omulator/msg/Message.hpp"
#include "omulator/msg/MessageType.hpp"
#include "omulator/util/TypeString.hpp"

#include <pybind11/embed.h>
#include <pybind11/iostream.h>

#include <cassert>
#include <memory>
#include <stdexcept>
#include <string>

using namespace pybind11::literals;

using omulator::di::TypeHash;
using omulator::util::TypeString;

namespace {
/**
 * The Python interpreter does NOT play well with multiple external threads, so we mark these as
 * thread_local to help limit the interpreter to a single thread. We also need a little bit of a
 * hack here to get our external dependencies into the scope of the PYBIND_* macros which need to be
 * at file scope, so we fill out these namespace scope references in Interpreter's constructor. The
 * scoped_interpreter needs a dtor, so we use a unique_ptr which, as a thread_local, will be
 * destructed on thread exit, while the Injector is just a non-owning reference, hence the raw
 * pointer.
 */
thread_local std::unique_ptr<pybind11::scoped_interpreter> pGuard    = nullptr;
thread_local omulator::di::Injector                       *gInjector = nullptr;
}  // namespace

PYBIND11_EMBEDDED_MODULE(omulator, m) {
  // `m` is a `py::module_` which is used to bind functions and classes
  m.def("log", [&](std::string msg) { gInjector->get<omulator::ILogger>().info(msg); });
}

namespace omulator {

bool       Interpreter::instanceFlag_{false};
std::mutex Interpreter::instanceLock_{};

Interpreter::Interpreter(di::Injector &injector)
  : Subsystem(
    injector.get<ILogger>(),
    TypeString<Interpreter>,
    injector.get<msg::MailboxRouter>(),
    TypeHash<Interpreter>,
    [&] {
      // This class doesn't stricly need to be a singleton, since multiple instantiations will
      // simply reset the Python interpreter. Furthermore, since each Subsystem initializes a new
      // thread and each Python interpreter is thread_local, this should never really happen. Having
      // said that, creating multiple interpreters with multiple GILs feels like a recipe for
      // disaster, so we throw if more than one interpreter is created at a time.
      std::scoped_lock lck{Interpreter::instanceLock_};
      if(Interpreter::instanceFlag_) {
        throw std::runtime_error("Attempted to initialize Python interpreter more than once");
      }
      assert(gInjector == nullptr && pGuard.get() == nullptr);

      // A little hacky, but this is how we can get our external dependencies into the scope of the
      // PYBIND_* macros which need to be at filescope.
      gInjector = &injector;
      pGuard.reset(new pybind11::scoped_interpreter);

      // The Python interpreter is stateful across exec calls, so we perform our initial import of
      // our custom module here.
      exec("import omulator as oml");

      Interpreter::instanceFlag_ = true;
    },
    [&] {
      std::scoped_lock lck{Interpreter::instanceLock_};
      assert(Interpreter::instanceFlag_ == true);
      pGuard.reset();
      Interpreter::instanceFlag_ = false;
    }),
    injector_(injector) { }

void Interpreter::exec(std::string str) { pybind11::exec(str); }

void Interpreter::message_proc(const msg::Message &msg) {
  if(msg.type == msg::MessageType::STDIN_STRING) {
    const std::string &execstr = msg.get_managed_payload<std::string>();

    try {
      exec(execstr);
    }
    catch(pybind11::error_already_set &e) {
      // N.B. that calling e.what from another thread is a bad idea, since it needs the GIL.
      injector_.get<ILogger>().error(e.what());
    }
  }
}

}  // namespace omulator
