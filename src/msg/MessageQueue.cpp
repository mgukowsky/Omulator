#include "omulator/msg/MessageQueue.hpp"

#include "omulator/util/to_underlying.hpp"

#include <cassert>
#include <sstream>

namespace omulator::msg {

MessageQueue::MessageQueue(Storage_t *pStorage, ILogger &logger)
  : pStorage_{pStorage}, logger_{logger}, sealed_{false}, valid_{true} { }

MessageQueue::MessageQueue(MessageQueue &&rhs) noexcept
  : pStorage_{rhs.pStorage_}, logger_{rhs.logger_}, sealed_{rhs.sealed_}, valid_{rhs.valid_} {
  rhs.mark_invalid();
}

MessageQueue &MessageQueue::operator=(MessageQueue &&rhs) noexcept {
  this->pStorage_ = rhs.pStorage_;
  this->sealed_   = rhs.sealed_;
  this->valid_    = rhs.valid_;

  rhs.mark_invalid();

  return *this;
}

void MessageQueue::clear() {
  for(auto &msg : pStorage_->storage) {
    if(msg.has_managed_payload() && msg.payload != 0) {
      free_managed_payload_(msg);
    }
  }

  seal();
}

void MessageQueue::free_managed_payload_(Message &msg) {
  assert(msg.has_managed_payload());

  di::TypeContainerBase *pPayload = reinterpret_cast<di::TypeContainerBase *>(msg.payload);
  assert(pPayload != nullptr);
  delete pPayload;

  msg.payload = 0;
}

void MessageQueue::mark_invalid() noexcept { valid_ = false; }

void MessageQueue::pump_msgs(const MessageCallback_t &callback) {
  if(!valid_) {
    logger_.error(
      "Attempted to call MessageQueue::pump_msgs() on a MessageQueue that is not valid");
    return;
  }

  if(!sealed_) {
    logger_.error("Attempted to call MessageQueue::pump_msgs() on a MessageQueue that has not been "
                  "sealed; no messages will be processed");

    return;
  }

  for(auto &msg : pStorage_->storage) {
    if(msg.type == MessageType::MSG_NULL) {
      /* no-op */
    }
    else if(util::to_underlying(msg.type) > util::to_underlying(MessageType::MSG_MAX)) {
      logger_.error("Message with type exceeding MSG_MAX detected by MessageQueue::pump_msgs; this "
                    "message will be dropped");
    }
    else {
      callback(msg);

      if(msg.has_managed_payload()) {
        free_managed_payload_(msg);
      }
    }
  }
}

MessageQueue::Storage_t *MessageQueue::release() noexcept {
  mark_invalid();

#ifndef NDEBUG
  // Check for memory leaks; a message with a managed payload which is not a nullptr is probably a
  // memory leak (MessageQueue::pump_msgs() should delete the payload and set the pointer to it to a
  // nullptr once the message is processed)!
  for(const auto &msg : pStorage_->storage) {
    if(msg.has_managed_payload()) {
      assert(reinterpret_cast<void *>(msg.payload) == nullptr);
    }
  }
#endif

  return pStorage_;
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

  if(!valid_) {
    logger_.error("Attempted to call MessageQueue::push() on a MessageQueue that is not valid");
    return;
  }

  if(sealed_) {
    std::stringstream ss;
    ss << "Could not push message because MessageQueue has already been sealed (type: "
       << util::to_underlying(type) << "; payload: " << payload << "); dropping message";
    logger_.error(ss.str().c_str());

    return;
  }

  pStorage_->storage.push_back({type, mflags, payload});
}

void MessageQueue::seal() noexcept { sealed_ = true; }

bool MessageQueue::sealed() const noexcept { return sealed_; }

bool MessageQueue::valid() const noexcept { return valid_; }

}  // namespace omulator::msg
