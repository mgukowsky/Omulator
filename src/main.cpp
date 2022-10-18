#include "omulator/main.hpp"

#include "omulator/App.hpp"
#include "omulator/CoreGraphicsEngine.hpp"
#include "omulator/IClock.hpp"
#include "omulator/ILogger.hpp"
#include "omulator/IWindow.hpp"
#include "omulator/InputHandler.hpp"
#include "omulator/Interpreter.hpp"
#include "omulator/PropertyMap.hpp"
#include "omulator/di/Injector.hpp"
#include "omulator/msg/MailboxRouter.hpp"
#include "omulator/props.hpp"
#include "omulator/util/CLIInput.hpp"
#include "omulator/util/CLIParser.hpp"
#include "omulator/util/exception_handler.hpp"

#include <thread>

using namespace std::chrono_literals;

namespace omulator {

int oml_main(const int argc, const char **argv) {
  try {
    di::Injector injector;
    di::Injector::installMinimalRules(injector);

    auto &cliparser = injector.get<util::CLIParser>();
    cliparser.parse_args(argc, argv);

    di::Injector::installDefaultRules(injector);

    IWindow &wnd = injector.get<IWindow>();
    // The window MUST be shown prior to creating the graphics backend, otherwise we may not be able
    // to associate the window with the graphics API.
    wnd.show();

    [[maybe_unused]] auto &testGraphicsEngine = injector.get<CoreGraphicsEngine>();
    if(injector.get<PropertyMap>().get_prop<bool>(props::INTERACTIVE).get()) {
      [[maybe_unused]] auto &cliinput    = injector.get<util::CLIInput>();
      [[maybe_unused]] auto &interpreter = injector.get<Interpreter>();
    }

    msg::MailboxReceiver mbrecv = injector.get<msg::MailboxRouter>().claim_mailbox<App>();
    msg::MailboxSender   testEngineMailbox =
      injector.get<msg::MailboxRouter>().get_mailbox<CoreGraphicsEngine>();
    IClock &clock = injector.get<IClock>();

    bool done = false;

    while(!done) {
      const TimePoint_t timeOfNextIteration = clock.now() + 16ms;

      mbrecv.recv(
        [&](const msg::Message &msg) {
          if(msg.type == msg::MessageType::APP_QUIT) {
            done = true;
          }
        },
        msg::RecvBehavior::NONBLOCK);
      wnd.pump_msgs();
      testEngineMailbox.send_single_message(msg::MessageType::RENDER_FRAME);

      clock.sleep_until(timeOfNextIteration);
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
