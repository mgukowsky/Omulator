#include "omulator/msg/MessageQueueFactory.hpp"

#include "concurrentqueue/concurrentqueue.h"

#include <cassert>
#include <sstream>

using moodycamel::ConcurrentQueue;

namespace omulator::msg {

struct MessageQueueFactory::Impl_ {
  Impl_()  = default;
  ~Impl_() = default;

  ConcurrentQueue<MessageQueue *> cQueue;
};

MessageQueueFactory::MessageQueueFactory(ILogger &logger) : logger_{logger}, numActiveQueues_{0} { }

MessageQueueFactory::~MessageQueueFactory() {
  MessageQueue *pQueue = nullptr;

  U64 i = 0;
  while(impl_->cQueue.try_dequeue(pQueue)) {
    assert(pQueue != nullptr);
    delete pQueue;
    ++i;
  }

  if(numActiveQueues_.load(std::memory_order_acquire) != i) {
    std::stringstream ss;
    ss << "MessageQueue* memory leak: MessageQueueFactory expected to destory " << numActiveQueues_
       << " MessageQueues, but instead destroyed " << i;
    logger_.error(ss.str().c_str());
  }
}

MessageQueue *MessageQueueFactory::get() noexcept {
  MessageQueue *pQueue = nullptr;
  if(impl_->cQueue.try_dequeue(pQueue)) {
    return pQueue;
  }
  else {
    numActiveQueues_.fetch_add(1, std::memory_order_acq_rel);
    return new MessageQueue(logger_);
  }
}

void MessageQueueFactory::submit(MessageQueue *pQueue) noexcept { impl_->cQueue.enqueue(pQueue); }

}  // namespace omulator::msg
