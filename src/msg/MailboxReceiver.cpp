#include "omulator/msg/MailboxReceiver.hpp"

namespace omulator::msg {

MailboxReceiver::MailboxReceiver(MailboxEndpoint &endpoint) : endpoint_(endpoint) { }

void MailboxReceiver::off(const MessageType type) { endpoint_.off(type); }

void MailboxReceiver::on(const MessageType type, std::function<void(void)> callback) {
  endpoint_.on(type, [callback]([[maybe_unused]] const Message &msg) { callback(); });
}

void MailboxReceiver::recv(RecvBehavior recvBehavior) { endpoint_.recv(recvBehavior); }

}  // namespace omulator::msg
