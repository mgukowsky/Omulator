#include "omulator/main.hpp"

#include "omulator/App.hpp"
#include "omulator/ILogger.hpp"
#include "omulator/IWindow.hpp"
#include "omulator/InputHandler.hpp"
#include "omulator/Interpreter.hpp"
#include "omulator/di/Injector.hpp"
#include "omulator/msg/MailboxRouter.hpp"
#include "omulator/util/CLIInput.hpp"
#include "omulator/util/CLIParser.hpp"
#include "omulator/util/exception_handler.hpp"

#include <thread>

using namespace std::chrono_literals;

namespace omulator {

int oml_main(const int argc, const char **argv) {
  try {
    di::Injector injector;
    di::Injector::installDefaultRules(injector);

    auto &cliparser = injector.get<util::CLIParser>();
    cliparser.parse_args(argc, argv);
    [[maybe_unused]] auto &cliinput    = injector.get<util::CLIInput>();
    [[maybe_unused]] auto &interpreter = injector.get<Interpreter>();

    msg::MailboxReceiver mbrecv = injector.get<msg::MailboxRouter>().claim_mailbox<App>();

    IWindow &wnd = injector.get<IWindow>();
    wnd.show();

    bool done = false;

    while(!done) {
      mbrecv.recv(
        [&](const msg::Message &msg) {
          if(msg.type == msg::MessageType::APP_QUIT) {
            done = true;
          }
        },
        msg::RecvBehavior::NONBLOCK);
      wnd.pump_msgs();
      std::this_thread::sleep_for(16ms);
    }
  }

  // Top level exception handler
  catch(...) {
    omulator::util::exception_handler();
  }

  return 0;
}
}  // namespace omulator

int main(const int argc, const char **argv) { return omulator::oml_main(argc, argv); }
