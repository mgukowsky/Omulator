#pragma once

#include <exception>

namespace omulator::util {
void exception_handler() noexcept { std::rethrow_exception(std::current_exception()); }
}  // namespace omulator::util
