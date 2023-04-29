#include "omulator/msg/MailboxSender.hpp"

namespace omulator::msg {

MailboxSender::MailboxSender(MailboxEndpoint &endpoint) : endpoint_(endpoint) { }

MessageQueue MailboxSender::get_mq() noexcept { return endpoint_.get_mq(); }

void MailboxSender::send(MessageQueue &mq) { endpoint_.send(mq); }
}  // namespace omulator::msg
