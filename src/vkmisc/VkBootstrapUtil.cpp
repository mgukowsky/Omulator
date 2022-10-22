#include "omulator/vkmisc/VkBootstrapUtil.hpp"

namespace omulator::vkmisc {

void vk_fatal(ILogger &logger, const char *msg) {
  std::string errmsg = "Fatal Vulkan error: ";
  errmsg += msg;
  logger.error(errmsg);
  throw std::runtime_error(errmsg);
}

void vk_fatal(ILogger &logger, const std::string &msg) { vk_fatal(logger, msg.c_str()); }

}