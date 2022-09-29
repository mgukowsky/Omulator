#include "omulator/util/CLIParser.hpp"

#include "docopt.h"

#include <string>

namespace {
constexpr auto USAGE = R"(
Usage: omulator [-h | --help] [-i | --interactive]

-h --help         Show this help
-i --interactive  Accept input from stdin, which will be interpreted as Python code   
)";
}  // namespace

namespace omulator::util {
CLIParser::CLIParser(ILogger &logger) : logger_(logger) { }

CLIParser::ArgMap_t CLIParser::parse_args(const int argc, const char **argv) {
  auto docparsemap = docopt::docopt(USAGE, {argv + 1, argv + argc});

  ArgMap_t argmap;
  for(const auto &[k, v] : docparsemap) {
    if(v.isBool()) {
      argmap.emplace(k, v.asBool());
    }
    else if(v.isLong()) {
      argmap.emplace(k, v.asLong());
    }
    else if(v.isString()) {
      argmap.emplace(k, v.asString());
    }
    else {
      std::string msg = "Failed to parse arg: ";
      msg += k;
      logger_.warn(msg);
    }
  }

  return argmap;
}
}  // namespace omulator::util