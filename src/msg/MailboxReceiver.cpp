#include "omulator/msg/MailboxReceiver.hpp"

namespace omulator::msg {

MailboxReceiver::MailboxReceiver(MailboxEndpoint &endpoint) : endpoint_(endpoint) { }

void MailboxReceiver::recv(const MessageCallback_t &callback, RecvBehavior recvBehavior) {
  endpoint_.recv(callback, recvBehavior);
}
}  // namespace omulator::msg
