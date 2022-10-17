#include "omulator/util/CLIInput.hpp"

#include "omulator/Interpreter.hpp"
#include "omulator/msg/Message.hpp"
#include "omulator/msg/MessageType.hpp"

#ifndef _MSC_VER
#include <readline/history.h>
#include <readline/readline.h>
#endif

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

namespace {
constexpr auto WHITESPACE_CHARS = " \a\b\f\n\r\t\v";
constexpr auto PROMPT           = "oml>";
}  // namespace

namespace omulator::util {

CLIInput::CLIInput(ILogger &logger, msg::MailboxRouter &mbrouter)
  : logger_(logger),
    msgSender_(mbrouter.get_mailbox<Interpreter>()),
    thrd_([this] { input_loop(); }) { }

void CLIInput::input_loop() {
  while(true) {
    std::string str;

// Use readline to block until a string is received, otherwise use the STL. Same behavior either
// way, we just get a more robust CLI with readline.
#ifndef _MSC_VER
    auto *stdinstr = ::readline(PROMPT);
    ::add_history(stdinstr);
    str = stdinstr;
    ::free(stdinstr);
#else
    std::cout << PROMPT;
    std::getline(std::cin, str);
#endif

    str = trim_string_(str);
    if(!str.empty()) {
      std::atomic_bool fence = false;

      auto mq = msgSender_.get_mq();
      mq->push_managed_payload<std::string>(msg::MessageType::STDIN_STRING, std::move(str));
      mq->push<std::atomic_bool *>(msg::MessageType::SIMPLE_FENCE, &fence);
      msgSender_.send(mq);

      // Wait on the fence to ensure that the CLI prompt only displays after the Interpreter has
      // printed its own output (the prompt may still be mangled if Omulator's logger is printing to
      // stdout in the meantime though, since the printing of the prompt to stdin is not
      // synchronized with Omulator's own logging functionality).
      fence.wait(false, std::memory_order_acquire);
    }
  }
}

std::string CLIInput::trim_string_(const std::string &str) {
  const auto firstValidChar = str.find_first_not_of(WHITESPACE_CHARS);
  const auto lastValidChar  = str.find_last_not_of(WHITESPACE_CHARS);
  if(firstValidChar == std::string::npos || lastValidChar == std::string::npos) {
    // The string is either empty of consists solely of whitespace
    return "";
  }
  else {
    return str.substr(firstValidChar, (lastValidChar + 1) - firstValidChar);
  }
}

}  // namespace omulator::util