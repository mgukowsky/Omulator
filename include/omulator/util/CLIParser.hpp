#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/oml_types.hpp"

#include <map>
#include <string>
#include <variant>

/**
 * Class which parses out command line args.
 */
namespace omulator::util {
class CLIParser {
public:
  using ArgMap_t =
    std::map<std::string, std::variant<const omulator::S64, const bool, const std::string>>;

  explicit CLIParser(ILogger &logger);

  /**
   * Parses command line args and returns the parsed values as a man of command line args mapped to
   * associated values.
   */
  ArgMap_t parse_args(const int argc, const char **argv);

private:
  ILogger &logger_;
};
}  // namespace omulator::util
