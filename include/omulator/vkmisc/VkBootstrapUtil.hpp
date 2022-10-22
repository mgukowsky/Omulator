#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/util/TypeString.hpp"

#include <VkBootstrap.h>

#include <sstream>
#include <stdexcept>
#include <string>

namespace omulator::vkmisc {

void vk_fatal(ILogger &logger, const char *msg);
void vk_fatal(ILogger &logger, const std::string &msg);

template<typename T>
// TODO: constrain me
void validate_vkb_return(ILogger &logger, T &retval) {
  if(!retval) {
    std::stringstream ss;
    ss << "vk-bootstrap return failure for type "
       << util::TypeString<T> << "; reason: " << retval.error().message();
    vk_fatal(logger, ss.str());
  }
}

}  // namespace omulator::vkmisc