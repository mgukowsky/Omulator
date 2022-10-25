#include "omulator/vkmisc/VkBootstrapUtil.hpp"

#include <sstream>

namespace {

/**
 * VkResult to string; from https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanTools.cpp
 */
const char *vk_error_string(const vk::Result errorCode) {
  switch(static_cast<VkResult>(errorCode)) {
#define VKSTRCONV(r) \
  case VK_##r:       \
    return #r
    VKSTRCONV(NOT_READY);
    VKSTRCONV(TIMEOUT);
    VKSTRCONV(EVENT_SET);
    VKSTRCONV(EVENT_RESET);
    VKSTRCONV(INCOMPLETE);
    VKSTRCONV(ERROR_OUT_OF_HOST_MEMORY);
    VKSTRCONV(ERROR_OUT_OF_DEVICE_MEMORY);
    VKSTRCONV(ERROR_INITIALIZATION_FAILED);
    VKSTRCONV(ERROR_DEVICE_LOST);
    VKSTRCONV(ERROR_MEMORY_MAP_FAILED);
    VKSTRCONV(ERROR_LAYER_NOT_PRESENT);
    VKSTRCONV(ERROR_EXTENSION_NOT_PRESENT);
    VKSTRCONV(ERROR_FEATURE_NOT_PRESENT);
    VKSTRCONV(ERROR_INCOMPATIBLE_DRIVER);
    VKSTRCONV(ERROR_TOO_MANY_OBJECTS);
    VKSTRCONV(ERROR_FORMAT_NOT_SUPPORTED);
    VKSTRCONV(ERROR_SURFACE_LOST_KHR);
    VKSTRCONV(ERROR_NATIVE_WINDOW_IN_USE_KHR);
    VKSTRCONV(SUBOPTIMAL_KHR);
    VKSTRCONV(ERROR_OUT_OF_DATE_KHR);
    VKSTRCONV(ERROR_INCOMPATIBLE_DISPLAY_KHR);
    VKSTRCONV(ERROR_VALIDATION_FAILED_EXT);
    VKSTRCONV(ERROR_INVALID_SHADER_NV);
#undef VKSTRCONV
    default:
      return "UNKNOWN_ERROR";
  }
}

}  // namespace

namespace omulator::vkmisc {

void validate_vk_return(ILogger &logger, std::string_view op, const vk::Result result) {
  // Don't care about these; just means the window has resized
  if(result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR) {
    return;
  }
  else if(result == vk::Result::eTimeout) {
    // Timeouts are not great, but tolerable. This enables us to continue execution in debugging
    // scenarios where timeouts may be exceeded, such as having the debugger paused on a breakpoint.
    // The downside to this approach is that it is incumbent on the end user to kill the program if
    // there is some sort of GPU driver failure that causes an operation to never return and
    // consequently lock up the application
    std::stringstream ss;
    ss << "Vulkan timeout detected in operation '" << op << "'";
    logger.warn(ss);
  }
  else if(result != vk::Result::eSuccess) {
    std::stringstream ss;
    ss << "Fatal vulkan error in op '" << op << "': " << vk_error_string(result);
    vk_fatal(logger, ss.str().c_str());
  }
}

void vk_fatal(ILogger &logger, const char *msg) {
  std::string errmsg = "Fatal Vulkan error: ";
  errmsg += msg;
  logger.error(errmsg);
  throw std::runtime_error(errmsg);
}

void vk_fatal(ILogger &logger, const std::string &msg) { vk_fatal(logger, msg.c_str()); }

}  // namespace omulator::vkmisc