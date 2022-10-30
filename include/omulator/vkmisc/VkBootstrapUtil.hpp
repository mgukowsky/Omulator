#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/util/TypeString.hpp"

#include <vulkan/vulkan.hpp>

#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace omulator::vkmisc {

constexpr auto OML_VK_VERSION = VK_API_VERSION_1_1;

void vk_fatal(ILogger &logger, const char *msg);
void vk_fatal(ILogger &logger, const std::string &msg);

void validate_vk_return(ILogger &logger, std::string_view op, const VkResult result);
void validate_vk_return(ILogger &logger, std::string_view op, const vk::Result result);

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