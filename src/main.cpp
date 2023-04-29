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

#include <filesystem>
#include <thread>

namespace {
constexpr auto FPS    = 60;
constexpr auto PERIOD = std::chrono::nanoseconds(1'000'000'000) / FPS;
}  // namespace

namespace omulator {

int oml_main(const int argc, const char **argv) {
  try {
    di::Injector injector;
    di::Injector::installMinimalRules(injector);

    auto &cliparser = injector.get<util::CLIParser>();
    cliparser.parse_args(argc, argv);

    di::Injector::installDefaultRules(injector);

    // TODO: find a better place to initialize these
    auto &propertyMap = injector.get<PropertyMap>();
    propertyMap.get_prop<std::string>(props::WORKING_DIR)
      .set(std::filesystem::current_path().string());
    propertyMap.get_prop<std::string>(props::RESOURCE_DIR)
      .set(std::filesystem::absolute(*argv).parent_path().string());

    auto &wnd = injector.get<IWindow>();
    // The window MUST be shown prior to creating the graphics backend, otherwise we may not be able
    // to associate the window with the graphics API.
    wnd.show();

    [[maybe_unused]] auto &testGraphicsEngine = injector.get<CoreGraphicsEngine>();
    // TODO: do this using System::make_subsystem_list
    testGraphicsEngine.start();

    if(injector.get<PropertyMap>().get_prop<bool>(props::INTERACTIVE).get()) {
      [[maybe_unused]] auto &cliinput    = injector.get<util::CLIInput>();
      [[maybe_unused]] auto &interpreter = injector.get<Interpreter>();
      // TODO: ditto
      interpreter.start();
    }

    msg::MailboxReceiver mbrecv = injector.get<msg::MailboxRouter>().claim_mailbox<App>();
    msg::MailboxSender   testEngineMailbox =
      injector.get<msg::MailboxRouter>().get_mailbox<CoreGraphicsEngine>();
    auto &clock = injector.get<IClock>();

    bool done = false;

    TimePoint_t timeOfNextIteration = clock.now();

    mbrecv.on(msg::MessageType::APP_QUIT, [&] { done = true; });

    while(!done) {
      mbrecv.recv(msg::RecvBehavior::NONBLOCK);
      wnd.pump_msgs();
      testEngineMailbox.send_single_message(msg::MessageType::RENDER_FRAME);

      timeOfNextIteration += PERIOD;

      // Account for drift/latency (delay in message processing above, late wakeup, etc.)
      const auto NOW = clock.now();
      if(NOW >= timeOfNextIteration) {
        timeOfNextIteration = NOW + PERIOD;
      }

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
