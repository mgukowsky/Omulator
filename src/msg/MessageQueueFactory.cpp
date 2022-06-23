#include "omulator/msg/MessageQueueFactory.hpp"

#include "concurrentqueue/concurrentqueue.h"

#include <cassert>

namespace omulator::msg {

struct MessageQueueFactory::Impl_ {
  Impl_()  = default;
  ~Impl_() = default;

  ConcurrentQueue<MessageQueue> cQueue;
};

MessageQueueFactory::MessageQueueFactory(ILogger &logger) : logger_{logger} { }

MessageQueueFactory::~MessageQueueFactory() {
  MessageQueue *pQueue = nullptr;
  while(impl_->cQueue.try_dequeue(pQueue)) {
    assert(pQueue != nullptr);
    delete pQueue;
  }
}

MessageQueue *MessageQueueFactory::get() noexcept {
  MessageQueue *pQueue = nullptr;
  if(impl_->cQueue.try_dequeue(pQueue)) {
    return pQueue;
  }
  else {
    return new MessageQueue(logger_);
  }
}

void MessageQueueFactory::submit(MessageQueue *pQueue) noexcept { impl_->cQueue.enqueue(pQueue); }

}  // namespace omulator::msg
