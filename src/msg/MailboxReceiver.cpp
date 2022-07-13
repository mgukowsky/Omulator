#include "omulator/msg/MailboxReceiver.hpp"

namespace omulator::msg {

MailboxReceiver::MailboxReceiver(MailboxEndpoint &endpoint) : endpoint_(endpoint) { }

void MailboxReceiver::recv(const MessageCallback_t &callback) { endpoint_.recv(callback); }
}  // namespace omulator::msg
