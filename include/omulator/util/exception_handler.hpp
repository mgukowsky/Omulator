#pragma once

#include "omulator/PrimitiveIO.hpp"

#include <cstdlib>
#include <exception>
#include <new>
#include <string>

namespace omulator::util {

/**
 * A Lippincott function for DRY, centralized exception handling.
 * See https://www.youtube.com/watch?v=-amJL3AyADI for background on this
 * design pattern.
 *
 * Alerts the user what went wrong, then causes the program to exit.
 */
[[noreturn]] void exception_handler() noexcept;

} /* namespace omulator::util */
