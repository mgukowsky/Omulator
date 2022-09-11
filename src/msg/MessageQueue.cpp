#include "omulator/msg/MessageQueue.hpp"

#include "omulator/util/to_underlying.hpp"

#include <cassert>
#include <sstream>

namespace omulator::msg {

MessageQueue::MessageQueue(ILogger &logger) : logger_{logger}, sealed_{false}, numNulls_{0} { }

std::size_t MessageQueue::num_null_msgs() const noexcept { return numNulls_; }

void MessageQueue::pump_msgs(const MessageCallback_t &callback) {
  if(!sealed_) {
    logger_.error("Attempted to call MessageQueue::pump_msgs() on a MessageQueue that has not been "
                  "sealed; no messages will be processed");

    return;
  }

  for(const auto &msg : queue_) {
    if(msg.type == MessageType::MSG_NULL) {
      ++numNulls_;
    }
    else {
      callback(msg);

      if(util::to_underlying(msg.mflags) & util::to_underlying(MessageFlagType::MANAGED_PTR)) {
        di::TypeContainerBase *pPayload = reinterpret_cast<di::TypeContainerBase *>(msg.payload);
        assert(pPayload != nullptr);
        delete pPayload;
      }
    }
  }
}

void MessageQueue::reset() noexcept {
  sealed_   = false;
  numNulls_ = 0;

  // N.B. that std::vector::clear() won't free any of the underlying storage, which is what we want,
  // since we can call reset() on a MessageQueue and submit it back to a pool
  queue_.clear();
}

void MessageQueue::push(const MessageType type) noexcept {
  push(type, MessageFlagType::FLAGS_NULL, 0);
}

void MessageQueue::push_impl_(const MessageType     type,
                              const MessageFlagType mflags,
                              const U64             payload) noexcept {
  // Necessary since we want the ability to have the payload for a single message point to a large
  // piece of data rather than copying it into multiple messages
  static_assert(sizeof(payload) >= sizeof(void *),
                "Message::payload must be large enough to accomodate a pointer");

  if(sealed_) {
    std::stringstream ss;
    ss << "Could not push message because MessageQueue has already been sealed (type: "
       << util::to_underlying(type) << "; payload: " << payload << "); dropping message";
    logger_.error(ss.str().c_str());

    return;
  }

  queue_.push_back({type, mflags, payload});
}

void MessageQueue::seal() noexcept { sealed_ = true; }

bool MessageQueue::sealed() const noexcept { return sealed_; }

}  // namespace omulator::msg
