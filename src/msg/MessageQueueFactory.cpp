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

MessageQueueFactory::MessageQueueFactory(ILogger &logger, const U64 id)
  : logger_{logger}, id_{id}, numActiveQueues_{0} { }

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
    ss << "MessageQueue::Storage_t* memory leak: MessageQueueFactory expected to destroy "
       << numActiveQueues_ << " MessageQueue::Storage_t, but instead destroyed " << i;
    logger_.error(ss.str().c_str());
  }
}

MessageQueue MessageQueueFactory::get() noexcept {
  MessageQueue::Storage_t *pStorage = nullptr;
  if(!(impl_->cQueue.try_dequeue(pStorage))) {
    numActiveQueues_.fetch_add(1, std::memory_order_acq_rel);
    pStorage = new MessageQueue::Storage_t(id_);
  }

  return {pStorage, logger_};
}

void MessageQueueFactory::submit(MessageQueue &mq) {
  if(!mq.valid()) {
    std::stringstream ss;
    ss << "MessageQueue::Storage_t* memory leak: attempted to submit a MessageQueue that is not "
          "valid; this can happen if the same MessageQueue instance is passed more than once to "
          "MessageQueueFactory::submit(); the MessageQueue will NOT be submitted";
    logger_.error(ss);

    return;
  }

  MessageQueue::Storage_t *pStorage = mq.release();

  if(pStorage->id != id_) {
    std::stringstream ss;
    ss << "MessageQueue::Storage_t* memory leak: attempted to submit a MessageQueue (factory id: "
       << pStorage->id << ") back to a MessageQueueFactory (factory id: " << id_
       << ") that did not create it";
    logger_.error(ss.str().c_str());
  }
  else {
    // N.B. that std::vector::clear() won't free any of the underlying storage, which is what we
    // want so that we can safely reuse it.
    pStorage->storage.clear();

    impl_->cQueue.enqueue(pStorage);
  }
}

}  // namespace omulator::msg
