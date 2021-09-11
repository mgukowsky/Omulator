#pragma once

#include <cstdlib>

[[noreturn]] void omulator::util::exception_handler() noexcept { std::exit(EXIT_FAILURE); }
