#pragma once

/**
 * "PrimitiveIO" functions are intended for use in situations where
 * conventional IO functionality cannot be guaranteed (i.e. during early
 * application initialization, handling a catastrophic error, etc.).
 *
 * These functions are platform-specific
 */
namespace omulator::PrimitiveIO {

  /**
   * Log to stdout
   */
  void log_msg(const char *msg);

  /**
   * Log to stdout and show the message in a dialog box
   */
  void alert_info(const char *msg);

  /**
   * Log to stderr and show the message in a dialog box
   */
  void alert_err(const char *msg);

} /* namespace omulator::PrimitiveIO */
