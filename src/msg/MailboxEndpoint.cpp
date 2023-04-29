#include "omulator/msg/MailboxEndpoint.hpp"

#include "omulator/util/to_underlying.hpp"

#include <cassert>
#include <sstream>

namespace omulator::msg {

MailboxEndpoint::MailboxEndpoint(const U64 id, ILogger &logger, MessageQueueFactory &mqfactory)
  : id_(id), claimed_(false), logger_(logger), mqfactory_(mqfactory) { }

void MailboxEndpoint::claim() noexcept { claimed_ = true; }

bool MailboxEndpoint::claimed() const noexcept { return claimed_; }

MessageQueue MailboxEndpoint::get_mq() noexcept { return mqfactory_.get(); }

void MailboxEndpoint::on(const MessageType type, const MessageCallback_t &callback) {
  std::scoped_lock lck{mtx_};

  if(callbacks_.contains(type)) {
    std::stringstream ss;
    ss << "Attempted to invoke MailboxEndpoint::on for message type " << util::to_underlying(type)
       << ", but there was already a callback registered; consider calling MailboxEndpoint::off "
          "first.";
    logger_.warn(ss);
    return;
  }

  callbacks_.emplace(type, callback);
}

void MailboxEndpoint::off(const MessageType type) {
  std::scoped_lock lck{mtx_};

  callbacks_.erase(type);
}

void MailboxEndpoint::recv(RecvBehavior recvBehavior) {
  if(recvBehavior == RecvBehavior::BLOCK) {
    std::unique_lock lck{mtx_};
    cv_.wait(lck, [this] { return !(queue_.empty()); });
  }

  while(true) {
    if(queue_.empty()) {
      break;
    }

    // TODO: this is very coarse-grained locking in that the mailbox will
    // remain locked until all messages in all pending queues have been processed...
    std::scoped_lock lck{mtx_};
    MessageQueue    &currentMQ = queue_.front();

    currentMQ.pump_msgs([this](const Message &msg) {
      auto it = callbacks_.find(msg.type);
      if(it != callbacks_.end()) {
        it->second(msg);
      }
      else {
        std::stringstream ss;
        ss << "Dropping message with type " << util::to_underlying(msg.type)
           << " because it had no registered callback; try adding one with MailboxEndpoint::on()";
        logger_.warn(ss);
      }
    });

    mqfactory_.submit(currentMQ);

    queue_.pop();
  }
}

void MailboxEndpoint::send(MessageQueue &mq) {
  mq.seal();
  {
    std::scoped_lock lck{mtx_};
    // N.B. we are copying the queue in
    queue_.push(mq);
    cv_.notify_all();
  }
}

}  // namespace omulator::msg
