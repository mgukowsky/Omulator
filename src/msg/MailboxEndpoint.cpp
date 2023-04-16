#include "omulator/msg/MailboxEndpoint.hpp"

#include "omulator/util/to_underlying.hpp"

#include <cassert>
#include <sstream>

namespace omulator::msg {

MailboxEndpoint::MailboxEndpoint(const U64 id, ILogger &logger, MessageQueueFactory &mqfactory)
  : id_(id), claimed_(false), logger_(logger), mqfactory_(mqfactory) { }

void MailboxEndpoint::claim() noexcept { claimed_ = true; }

bool MailboxEndpoint::claimed() const noexcept { return claimed_; }

MessageQueue *MailboxEndpoint::get_mq() noexcept { return mqfactory_.get(); }

void MailboxEndpoint::on(const MessageType type, const MessageCallback_t callback) {
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

  MessageQueue *pQueue = nullptr;

  while(true) {
    pQueue = pop_next_();

    if(pQueue == nullptr) {
      break;
    }

    std::scoped_lock lck{mtx_};

    pQueue->pump_msgs([this](const Message &msg) {
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

    mqfactory_.submit(pQueue);
  }
}

void MailboxEndpoint::send(MessageQueue *pQueue) {
  pQueue->seal();
  {
    std::scoped_lock lck{mtx_};
    queue_.push(pQueue);
    cv_.notify_all();
  }
}

MessageQueue *MailboxEndpoint::pop_next_() {
  std::scoped_lock lck{mtx_};

  if(queue_.empty()) {
    return nullptr;
  }
  else {
    MessageQueue *retval = queue_.front();
    queue_.pop();

    return retval;
  }
}

}  // namespace omulator::msg
