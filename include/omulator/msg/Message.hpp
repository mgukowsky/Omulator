#pragma once

#include "omulator/msg/MessageType.hpp"
#include "omulator/oml_types.hpp"

namespace omulator::msg {

/**
 * A message used to communicate between threads. Consists of a message type and an associated
 * payload.
 */
struct Message {
  Message() : Message{MessageType::MSG_NULL} { }
  Message(const MessageType typeArg) : Message{typeArg, 0} { }
  Message(const MessageType typeArg, const U64 payloadArg) : type{typeArg}, payload{payloadArg} { }

  const MessageType type;

  /**
   * N.B. that the payload should be large enough to hold a pointer, that way clients have the
   * option to point to a larger payload, as opposed to splitting a larger payload into multiple
   * smaller messages.
   */
  const U64 payload;
};

}  // namespace omulator::msg
