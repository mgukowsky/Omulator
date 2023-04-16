#include "omulator/Subsystem.hpp"

#include "omulator/util/exception_handler.hpp"
#include "omulator/util/to_underlying.hpp"

#include <sstream>
#include <string>

namespace omulator {

Subsystem::Subsystem(ILogger                  &logger,
                     std::string_view          name,
                     msg::MailboxRouter       &mbrouter,
                     const msg::MailboxToken_t mailboxToken,
                     std::function<void()>     onStart,
                     std::function<void()>     onEnd)
  : logger_{logger},
    receiver_{mbrouter.claim_mailbox(mailboxToken)},
    name_{name},
    sender_{mbrouter.get_mailbox(mailboxToken)},
    startSignal_{false},
    thrd_{&Subsystem::thrd_proc_, this, onStart, onEnd} {
  receiver_.on(msg::MessageType::POKE, [] { /* no-op */ });
  std::string str("Creating subsystem: ");
  str += name_;
  logger_.info(str.c_str());
}

Subsystem::~Subsystem() {
  // Awkward looking, but necessary in case we have a derived Subsystem constructor that throws an
  // exception before it can call start(). We call stop() first to ensure that the stop token is
  // set, then call start() to unblock the startSignal_ in case it needs to be signaled.
  stop();
  start();

  sender_.send_single_message(msg::MessageType::POKE);
}

std::string_view Subsystem::name() const noexcept { return name_; }

void Subsystem::start() {
  startSignal_.store(true, std::memory_order_release);
  startSignal_.notify_all();
}

void Subsystem::stop() { thrd_.request_stop(); }

void Subsystem::thrd_proc_(std::function<void()> onStart, std::function<void()> onEnd) {
  // Wrap each thread in its own exception handler
  try {
    startSignal_.wait(false, std::memory_order_acquire);
    onStart();
    auto stoken = thrd_.get_stop_token();
    while(!stoken.stop_requested()) {
      receiver_.recv();
    }
    onEnd();
  }
  catch(...) {
    util::exception_handler();
  }
}
}  // namespace omulator
