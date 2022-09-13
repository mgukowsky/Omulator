#pragma once

#include "omulator/di/TypeHash.hpp"
#include "omulator/di/TypeMap.hpp"
#include "omulator/msg/MessageType.hpp"
#include "omulator/oml_types.hpp"
#include "omulator/util/to_underlying.hpp"

#include <cassert>

namespace omulator::msg {

/**
 * A message used to communicate between threads. Consists of a message type and an associated
 * payload.
 */
struct Message {
  Message() : Message{MessageType::MSG_NULL} { }
  Message(const MessageType typeArg) : Message{typeArg, MessageFlagType::FLAGS_NULL, 0} { }
  Message(const MessageType typeArg, const MessageFlagType mflagsArg, const U64 payloadArg)
    : type{typeArg}, mflags{mflagsArg}, payload{payloadArg} { }

  const MessageType     type;
  const MessageFlagType mflags;

  /**
   * N.B. that the payload should be large enough to hold a pointer, that way clients have the
   * option to point to a larger payload, as opposed to splitting a larger payload into multiple
   * smaller messages.
   */
  const U64 payload;

  /**
   * Convenience function to return a read-only reference to a MessageQueue-managed payload.
   */
  template<typename T>
  const T &get_managed_payload() const {
    assert(reinterpret_cast<void *>(payload) != nullptr);
    assert(util::to_underlying(mflags) & util::to_underlying(MessageFlagType::MANAGED_PTR));

    auto *pCtr = reinterpret_cast<di::TypeContainer<const T> *>(payload);

    // Type safety assertion for the otherwise blind reinterpret_case we're doing here.
    // TypeContainerBase::identity() will return the TypeHash that was set upon the container's
    // creation, and should match the type T that we're casting to provided that the message is
    // being interpreted correctly.
    // TODO: maybe should throw instead of just assert?
    assert(di::TypeHash<std::decay_t<T>> == pCtr->identity());

    return pCtr->ref();
  }
};

}  // namespace omulator::msg
