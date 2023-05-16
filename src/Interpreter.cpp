#include "omulator/Interpreter.hpp"

#include "omulator/App.hpp"
#include "omulator/ILogger.hpp"
#include "omulator/PropertyMap.hpp"
#include "omulator/graphics/CoreGraphicsEngine.hpp"
#include "omulator/msg/MailboxRouter.hpp"
#include "omulator/msg/Message.hpp"
#include "omulator/msg/MessageType.hpp"
#include "omulator/util/TypeHash.hpp"
#include "omulator/util/TypeString.hpp"

#include "sol/state.hpp"

#include <cassert>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

using omulator::util::TypeHash;
using omulator::util::TypeString;

namespace omulator {

struct Interpreter::Impl_ {
  Impl_()  = default;
  ~Impl_() = default;
  sol::state lua;
};

Interpreter::Interpreter(di::Injector &injector)
  : Subsystem(
    injector.get<ILogger>(),
    TypeString<Interpreter>,
    injector.get<msg::MailboxRouter>(),
    TypeHash<Interpreter>,
    [&] {
      auto &lua = impl_->lua;
      lua.open_libraries(sol::lib::base, sol::lib::package);

      // Set up namespace
      auto oml = lua["oml"].get_or_create<sol::table>();

      oml.set_function("get_prop", [&](std::string prop) {
        return injector_.get<PropertyMap>().get_prop_variant(prop);
      });

      oml.set_function("log", [&](std::string_view str) {
        std::stringstream ss;
        ss << "(Lua) " << str;
        logger_.info(ss);
      });

      oml.set_function("set_prop",
                       [&](std::string_view prop, PropertyMap::PropVariant_t val) {
        injector_.get<PropertyMap>().set_prop_variant(prop.data(), val);
                       });

      oml.set_function("set_vertex_shader", [&](std::string shader) {
        auto sender =
          injector.get<msg::MailboxRouter>().get_mailbox<omulator::graphics::CoreGraphicsEngine>();
        auto mq     = sender.get_mq();
        mq.push_managed_payload<std::string>(omulator::msg::MessageType::SET_VERTEX_SHADER, shader);
        sender.send(mq);
      });

      oml.set_function("shutdown", [&] {
        injector_.get<msg::MailboxRouter>().get_mailbox<App>().send_single_message(
          msg::MessageType::APP_QUIT);
      });
    },
    [&] {}),
    injector_(injector),
    logger_(injector_.get<ILogger>()) {
  receiver_.on_managed_payload<std::string>(msg::MessageType::STDIN_STRING,
                                            [this](const std::string &execstr) {
                                              exec(execstr);
                                            });
  receiver_.on_unmanaged_payload<std::atomic_bool>(msg::MessageType::SIMPLE_FENCE,
                                                   [](std::atomic_bool &fence) {
                                                     fence.store(true, std::memory_order_release);
                                                     fence.notify_all();
                                                   });
}

void Interpreter::exec(std::string_view str) {
  auto result = impl_->lua.safe_script(str, sol::script_pass_on_error);
  if(!result.valid()) {
    std::stringstream ss;
    sol::error        error = result;
    ss << "Lua error: " << error.what();
    logger_.error(ss);
  }
}

}  // namespace omulator
