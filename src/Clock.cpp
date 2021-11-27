#include "omulator/Clock.hpp"

#include <chrono>

namespace omulator {

TimePoint_t now() const noexcept override { return std::chrono::steady_clock::now(); }

}