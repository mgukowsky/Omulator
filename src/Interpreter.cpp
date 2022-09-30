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
#include <string>

using namespace pybind11::literals;

using omulator::di::TypeHash;
using omulator::util::TypeString;

namespace {
/**
 * The Python interpreter does NOT play well with multiple external threads, so we mark these as
 * thread_local to limit the interpreter to a single thread. We also need a little bit of a hack
 * here to get our external dependencies into the scope of the PYBIND_* macros which need to be at
 * file scope, so we fill out these namespace scope references in Interpreter's constructor. The
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

Interpreter::Interpreter(di::Injector &injector)
  : Subsystem(
    injector.get<ILogger>(),
    TypeString<Interpreter>,
    injector.get<msg::MailboxRouter>(),
    TypeHash<Interpreter>,
    [&] {
      // This class doesn't stricly need to be a singleton, since multiple instantiations will
      // simply reset the Python interpreter. Furthermore, since each Subsystem initializes a new
      // thread and each Python interpreter is thread_local, this should never really happen.
      assert(gInjector == nullptr && pGuard.get() == nullptr);

      // A little hacky, but this is how we can get our external dependencies into the scope of the
      // PYBIND_* macros which need to be at filescope.
      gInjector = &injector;
      pGuard.reset(new pybind11::scoped_interpreter);

      // The Python interpreter is stateful across exec calls, so we perform our initial import of
      // our custom module here.
      exec("import omulator");
    },
    [&] {}),
    injector_(injector) { }

void Interpreter::exec(std::string str) { pybind11::exec(str); }

void Interpreter::message_proc(const msg::Message &msg) {
  if(msg.type == msg::MessageType::STDIN_STRING) {
    const std::string &execstr = msg.get_managed_payload<std::string>();

    try {
      exec(execstr);
    }
    catch(pybind11::error_already_set &e) {
      // TODO: e.what() seems to cause issues with this exception type, so we use this member
      // function instead. The downside here is that this prints the traceback to stdout (stderr?),
      // so a way to capture the traceback in a string instead would be preferable.
      e.discard_as_unraisable(execstr.c_str());
      injector_.get<ILogger>().error("Python error");
    }
  }
}

}  // namespace omulator
