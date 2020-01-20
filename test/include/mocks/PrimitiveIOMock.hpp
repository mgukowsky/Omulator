#pragma once

#include <iostream>

/**
 * Mocks for primitive i/o functions. N.B. these still write to stdout/stderr
 * b/c we have test cases which monitor stdout/stderr.
 */
namespace omulator::PrimitiveIO {

void log_msg(const char *msg) { std::cout << msg; }

void log_err(const char *msg) {
  std::cout << msg;
  std::cerr << msg;
}

void alert_info(const char *msg) { log_msg(msg); }

void alert_err(const char *msg) { log_err(msg); }

} /* namespace omulator::PrimitiveIO */
