#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/msg/MessageQueue.hpp"
#include "omulator/util/Pimpl.hpp"

#include <atomic>

namespace omulator::msg {

/**
 * Specialized factory for MessageQueues to allow for fast, concurrent re-use of the MessageQueues
 * and their internal storage, thereby avoiding frequent allocations/deallocations. This specialized
 * use case is the reason why we need this factory instead of creating MessageQueues through an
 * Injector. Essentially a memory pool.
 */
class MessageQueueFactory {
public:
  explicit MessageQueueFactory(ILogger &logger);

  /**
   * Iterate and free each MessageQueue in an unsynchronized manner, meaning that this destructor
   * should only be invoked when all threads have submitted their MessageQueues back to the pool.
   */
  ~MessageQueueFactory();

  /**
   * Retrieve a pointer to a MessageQueue that is ready to accept messages.
   */
  MessageQueue *get() noexcept;

  /**
   * Relinquish a MessageQueue back into the pool of available queues. Upon submission, reset() will
   * be invoked on the MessageQueue. N.B. that forgetting to submit back to this class constitutes a
   * memory leak! Once submitted, any references to the submitted MessageQueue will no longer be
   * valid.
   */
  void submit(MessageQueue *pQueue) noexcept;

private:
  struct Impl_;
  util::Pimpl<Impl_> impl_;

  ILogger &logger_;

  std::atomic<U64> numActiveQueues_;
};

}  // namespace omulator::msg
