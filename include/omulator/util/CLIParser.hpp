#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/PropertyMap.hpp"
#include "omulator/oml_types.hpp"

#include <map>
#include <string>

/**
 * Class which parses out command line args.
 */
namespace omulator::util {
class CLIParser {
public:
  explicit CLIParser(ILogger &logger, PropertyMap &propertyMap);

  /**
   * Parses command line args and stores the parsed arguments into propertyMap.
   */
  void parse_args(const int argc, const char **argv);

private:
  ILogger     &logger_;
  PropertyMap &propertyMap_;
};
}  // namespace omulator::util
