#include "omulator/msg/MessageQueueFactory.hpp"

#include "concurrentqueue/concurrentqueue.h"

#include <cassert>
#include <sstream>

using moodycamel::ConcurrentQueue;

namespace omulator::msg {

struct MessageQueueFactory::Impl_ {
  Impl_()  = default;
  ~Impl_() = default;

  ConcurrentQueue<MessageQueue::Storage_t *> cQueue;
};

MessageQueueFactory::MessageQueueFactory(ILogger &logger) : logger_{logger}, numActiveQueues_{0} { }

MessageQueueFactory::~MessageQueueFactory() {
  MessageQueue::Storage_t *pStorage = nullptr;

  U64 i = 0;
  while(impl_->cQueue.try_dequeue(pStorage)) {
    assert(pStorage != nullptr);
    delete pStorage;
    ++i;
  }

  if(numActiveQueues_.load(std::memory_order_acquire) != i) {
    std::stringstream ss;
    ss << "MessageQueue::Storage_t* memory leak: MessageQueueFactory expected to destory "
       << numActiveQueues_ << " MessageQueue::Storage_t, but instead destroyed " << i;
    logger_.error(ss.str().c_str());
  }
}

MessageQueue MessageQueueFactory::get() noexcept {
  MessageQueue::Storage_t *pStorage = nullptr;
  if(!(impl_->cQueue.try_dequeue(pStorage))) {
    numActiveQueues_.fetch_add(1, std::memory_order_acq_rel);
    pStorage = new MessageQueue::Storage_t;
  }

  return {pStorage, logger_};
}

void MessageQueueFactory::submit(MessageQueue &mq) noexcept { impl_->cQueue.enqueue(mq.reset()); }

}  // namespace omulator::msg
