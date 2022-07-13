#include "omulator/msg/MailboxEndpoint.hpp"

#include "concurrentqueue/concurrentqueue.h"

#include <cassert>

using moodycamel::ConcurrentQueue;

namespace omulator::msg {

struct MailboxEndpoint::Impl_ {
  Impl_()  = default;
  ~Impl_() = default;

  ConcurrentQueue<MessageQueue *> cQueue;
};

MailboxEndpoint::MailboxEndpoint(const U64 id, MessageQueueFactory &mqfactory)
  : id_(id), claimed_(false), mqfactory_(mqfactory) { }

// Need dtor declared in header and defined here to make Pimpl happy
MailboxEndpoint::~MailboxEndpoint() { }

void MailboxEndpoint::claim() noexcept { claimed_ = true; }

bool MailboxEndpoint::claimed() const noexcept { return claimed_; }

MessageQueue *MailboxEndpoint::get_mq() noexcept { return mqfactory_.get(); }

void MailboxEndpoint::send(MessageQueue *pQueue) {
  pQueue->seal();
  impl_->cQueue.enqueue(pQueue);
}

void MailboxEndpoint::recv(const MessageCallback_t &callback) {
  MessageQueue *pQueue = nullptr;

  while(impl_->cQueue.try_dequeue(pQueue)) {
    assert(pQueue != nullptr);
    pQueue->pump_msgs(callback);
    mqfactory_.submit(pQueue);
  }
}

}  // namespace omulator::msg
