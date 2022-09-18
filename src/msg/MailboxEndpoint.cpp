#include "omulator/msg/MailboxEndpoint.hpp"

#include <cassert>

namespace omulator::msg {

MailboxEndpoint::MailboxEndpoint(const U64 id, MessageQueueFactory &mqfactory)
  : id_(id), claimed_(false), mqfactory_(mqfactory) { }

void MailboxEndpoint::claim() noexcept { claimed_ = true; }

bool MailboxEndpoint::claimed() const noexcept { return claimed_; }

MessageQueue *MailboxEndpoint::get_mq() noexcept { return mqfactory_.get(); }

void MailboxEndpoint::recv(const MessageCallback_t &callback) {
  {
    std::unique_lock lck{mtx_};
    cv_.wait(lck, [this] { return !(queue_.empty()); });
  }

  MessageQueue *pQueue = nullptr;

  while(true) {
    pQueue = pop_next_();

    if(pQueue == nullptr) {
      break;
    }

    pQueue->pump_msgs(callback);
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
