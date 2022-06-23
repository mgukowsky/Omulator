#pragma once

#include "omulator/oml_types.hpp"

namespace omulator::msg {

/**
 * Indicates the type of a given message, which can be used to determine an action to take as well
 * as how/if to interpret a Message's payload.
 *
 * N.B. that ALL message types are defined here in this enum, to ensure that all message types are
 * globally unique. We _could_ define a separate enum for each class that receives messages, but
 * ensuring that all messages use globally unique values under the hood helps prevent
 * reinterpretation issues that could arise if were to cast these distint enum types to their
 * underlying values and then pass them around (e.g. enum with an underlying value of 1 means one
 * thing to one class and something else to another, which could cause tough-to-debug issues when
 * the associated payload is misinterpreted as a result).
 */
enum class MessageType : U64 {
  /**
   * Will not be processed if received.
   */
  MSG_NULL = 0,

  DEMO_MSG_A,
  DEMO_MSG_B,
  DEMO_MSG_C,

  MSG_MAX,
};

}  // namespace omulator::msg
