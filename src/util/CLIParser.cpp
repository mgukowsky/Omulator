#include "omulator/util/CLIParser.hpp"

#include "omulator/props.hpp"

#include "docopt.h"

#include <string>
#include <unordered_map>

namespace {

/**
 * Maps CLI switches to internal property names.
 */
const std::map<std::string_view, std::string_view> cliArgToProp{
  {"--headless",    omulator::props::HEADLESS   },
  {"--interactive", omulator::props::INTERACTIVE},
};

/**
 * Docopt magic string; see repo for explanation.
 *
 * N.B. that, because we are adding the -h/--help flags, docopt will cause our app to exit
 * prematurely and print the help message if either flag is provided.
 */
constexpr auto USAGE = R"(
Usage: omulator [--help] [--headless] [--interactive]

--help         Show this help
--headless     Run without a GUI window
--interactive  Accept input from stdin, which will be interpreted as Python code   
)";
}  // namespace

namespace omulator::util {
CLIParser::CLIParser(ILogger &logger, PropertyMap &propertyMap)
  : logger_(logger), propertyMap_(propertyMap) { }

void CLIParser::parse_args(const int argc, const char **argv) {
  auto docparsemap = docopt::docopt(USAGE, {argv + 1, argv + argc});

  for(const auto &[k, v] : docparsemap) {
    // This is a magic switch entirely managed by docopt.cpp
    if(k == "--help") {
      continue;
    }

    auto cliArgToPropEntry = cliArgToProp.find(k);
    if(cliArgToPropEntry != cliArgToProp.end()) {
      std::string propName = std::string(cliArgToPropEntry->second);
      if(v.isBool()) {
        propertyMap_.get_prop<bool>(propName).set(v.asBool());
      }
      else if(v.isLong()) {
        propertyMap_.get_prop<S64>(propName).set(v.asLong());
      }
      else if(v.isString()) {
        propertyMap_.get_prop<std::string>(propName).set(v.asString());
      }
      else {
        // Should never happen unless something is set up incorrectly in the USAGE string
        std::string msg = "Failed to parse arg: ";
        msg += k;
        logger_.warn(msg);
      }
    }
    else {
      std::string msg = "Unknown CLI switch received: ";
      msg += k;
      logger_.warn(msg);
    }
  }
}
}  // namespace omulator::util
