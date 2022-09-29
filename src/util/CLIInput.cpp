#include "omulator/util/CLIInput.hpp"

#include "omulator/App.hpp"
#include "omulator/msg/Message.hpp"
#include "omulator/msg/MessageType.hpp"

#include <iostream>
#include <string>
#include <utility>

namespace omulator::util {

CLIInput::CLIInput(msg::MailboxRouter &mbrouter)
  : msgSender_(mbrouter.get_mailbox<App>()), thrd_([this] { input_loop(); }) { }

void CLIInput::input_loop() {
  while(true) {
    std::string str;
    std::getline(std::cin, str);

    auto mq = msgSender_.get_mq();
    mq->push_managed_payload<std::string>(msg::MessageType::STDIN_STRING, std::move(str));
    msgSender_.send(mq);
  }
}
}  // namespace omulator::util