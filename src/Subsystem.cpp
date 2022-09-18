#include "omulator/Subsystem.hpp"

#include <string>

namespace omulator {

Subsystem::Subsystem(ILogger             &logger,
                     std::string_view     name,
                     msg::MailboxReceiver receiver,
                     msg::MailboxSender   sender)
  : logger_{logger},
    name_{name},
    receiver_{receiver},
    sender_{sender},
    thrd_{&Subsystem::thrd_proc_, this} {
  std::string str("Creating subsystem: ");
  str += name_;
  logger_.info(str.c_str());
}

Subsystem::~Subsystem() {
  thrd_.request_stop();

  auto mq = sender_.get_mq();
  mq->push(msg::MessageType::POKE);
  mq->seal();
  sender_.send(mq);
}

std::string_view Subsystem::name() const noexcept { return name_; }

void Subsystem::message_proc(const msg::Message &msg) {
  if(msg.type == msg::MessageType::POKE) {
    /* no-op */
  }
}

void Subsystem::thrd_proc_() {
  auto stoken = thrd_.get_stop_token();
  while(!stoken.stop_requested()) {
    receiver_.recv([this](const msg::Message &msg) { message_proc(msg); });
  }
}
}  // namespace omulator
