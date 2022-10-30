#include "omulator/Interpreter.hpp"

#include "omulator/App.hpp"
#include "omulator/CoreGraphicsEngine.hpp"
#include "omulator/ILogger.hpp"
#include "omulator/PropertyMap.hpp"
#include "omulator/msg/MailboxRouter.hpp"
#include "omulator/msg/Message.hpp"
#include "omulator/msg/MessageType.hpp"
#include "omulator/util/TypeHash.hpp"
#include "omulator/util/TypeString.hpp"

#include <pybind11/attr.h>
#include <pybind11/embed.h>
#include <pybind11/iostream.h>
#include <pybind11/stl.h>

#include <cassert>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

using namespace pybind11::literals;

using pybind11::doc;

using omulator::util::TypeHash;
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

/**
 * The code which the Python interpreter will execute upon startup.
 */
constexpr auto STARTUP_STR = R"(
import io
import sys
import omulator as oml

# Redirect Python's stdio to strings.
# To reset, do `setattr(sys, "stdout"|"stderr", sys.__stdout__|sys.__stderr)`
STDOUT_SINK = io.StringIO()
STDERR_SINK = io.StringIO()

setattr(sys, "stdout", STDOUT_SINK)
setattr(sys, "stderr", STDERR_SINK)
)";

/**
 * Incantation to extract and reset stdio streams.
 *
 * From https://stackoverflow.com/a/4330829
 */
constexpr auto RESET_STDIO_STR = R"(
stdout_capture = STDOUT_SINK.getvalue()
STDOUT_SINK.truncate(0)
STDOUT_SINK.seek(0)

stderr_capture = STDERR_SINK.getvalue()
STDERR_SINK.truncate(0)
STDERR_SINK.seek(0)
)";

}  // namespace

PYBIND11_EMBEDDED_MODULE(omulator, m) {
  auto &logger      = gInjector->get<omulator::ILogger>();
  auto &mbrouter    = gInjector->get<omulator::msg::MailboxRouter>();
  auto &propertyMap = gInjector->get<omulator::PropertyMap>();

  m.def(
    "get_prop",
    [&](std::string prop) { return propertyMap.get_prop_variant(prop); },
    doc{"Get the value of a given property"});

  m.def(
    "log",
    [&](std::string msg) { logger.info(msg); },
    doc{"Log a string using Omulator's main logger"});

  m.def(
    "set_prop",
    // N.B. that bool is not included here; Python will send those over as S64's. Also note that
    // Python can decide to send over either a S64 or U64 depending on how big the value is, so we
    // have to handle such cases appropriately
    [&](std::string prop, omulator::PropertyMap::PropVariant_t val) {
      propertyMap.set_prop_variant(prop, val);
    },
    doc{"Set a property to a given value"});

  m.def(
    "set_vertex_shader",
    [&](std::string shader) {
      auto sender = mbrouter.get_mailbox<omulator::CoreGraphicsEngine>();
      auto mq     = sender.get_mq();
      mq->push_managed_payload<std::string>(omulator::msg::MessageType::SET_VERTEX_SHADER, shader);
      sender.send(mq);
    },
    doc{"Set the vertex shader to the specified file"});

  m.def(
    "shutdown",
    [&]() {
      mbrouter.get_mailbox<omulator::App>().send_single_message(
        omulator::msg::MessageType::APP_QUIT);
    },
    doc{"Gracefully shutdown the Omulator application"});
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

      // The Python interpreter is stateful across exec calls, so we perform our initial setup here.
      exec(STARTUP_STR);

      Interpreter::instanceFlag_ = true;
    },
    [&] {
      std::scoped_lock lck{Interpreter::instanceLock_};
      assert(Interpreter::instanceFlag_ == true);
      pGuard.reset();
      Interpreter::instanceFlag_ = false;
    }),
    injector_(injector),
    logger_(injector_.get<ILogger>()) {
  start_();
}

void Interpreter::exec(std::string str) {
  pybind11::exec(str);
  auto iocaptures = reset_stdio_();
  if(!(iocaptures.first.empty())) {
    std::string outmsg = "Python stdout capture: ";
    outmsg += iocaptures.first;
    logger_.info(outmsg);
  }
  if(!(iocaptures.second.empty())) {
    std::string errmsg = "Python stderr capture: ";
    errmsg += iocaptures.second;
    logger_.info(errmsg);
  }
}

void Interpreter::message_proc(const msg::Message &msg) {
  if(msg.type == msg::MessageType::STDIN_STRING) {
    const std::string &execstr = msg.get_managed_payload<std::string>();

    try {
      exec(execstr);
    }
    catch(pybind11::error_already_set &e) {
      // N.B. that calling e.what() from another thread is a bad idea, since it needs the GIL.
      logger_.error(e.what());
    }
  }
  else if(msg.type == msg::MessageType::SIMPLE_FENCE) {
    auto fence = reinterpret_cast<std::atomic_bool *>(msg.payload);
    fence->store(true, std::memory_order_release);
    fence->notify_all();
  }
  else {
    Subsystem::message_proc(msg);
  }
}

std::pair<std::string, std::string> Interpreter::reset_stdio_() {
  auto locals = pybind11::dict();
  pybind11::exec(RESET_STDIO_STR, pybind11::globals(), locals);

  return {locals["stdout_capture"].cast<std::string>(),
          locals["stderr_capture"].cast<std::string>()};
}

}  // namespace omulator
