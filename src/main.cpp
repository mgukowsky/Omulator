#include <iostream>

#include "omulator/main.hpp"
#include "omulator/PrimitiveIO.hpp"

int main([[maybe_unused]] const int argc, [[maybe_unused]] const char **argv) {
  omulator::PrimitiveIO::alert_info("Hi!");

  return 0;
}