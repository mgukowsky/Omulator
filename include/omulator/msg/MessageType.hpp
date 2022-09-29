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
enum class MessageType : U32 {
  /**
   * Will not be processed if received.
   */
  MSG_NULL = 0,

  /**
   * Intended as a no-op message which serves no purpose other than to wake a thread that is waiting
   * on a recv call.
   */
  POKE,

  /**
   * A request to stop the application.
   */
  APP_QUIT,

  /**
   * A string received on STDIN.
   */
  STDIN_STRING,

  /**
   * Placeholder messages used for testing and diagnostic purposes.
   */
  DEMO_MSG_A,
  DEMO_MSG_B,
  DEMO_MSG_C,

  /**
   * Message types which exceed this value should be ignored.
   */
  MSG_MAX,
};

/**
 * Bit flags which contain hints about the payload.
 */
enum class MessageFlagType : U32 {
  FLAGS_NULL = 0,

  /**
   * If present, then the payload is a pointer which will be deleted after the callback for the
   * MessageType (if present) is invoked.
   */
  MANAGED_PTR = 0x01,

  FLAGS_MAX,
};

}  // namespace omulator::msg
