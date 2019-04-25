#include <iostream>

#include "omulator/PrimitiveIO.hpp"
#include "omulator/main.hpp"

int main([[maybe_unused]] const int argc, [[maybe_unused]] const char **argv) {
  omulator::PrimitiveIO::alert_info("Hi!");

  return 0;
}
