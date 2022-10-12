#include "omulator/VulkanBackend.hpp"

#include "omulator/props.hpp"
#include "omulator/util/TypeString.hpp"
#include "omulator/util/reinterpret.hpp"

#include <VkBootstrap.h>

#include <cassert>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace { }  // namespace

namespace omulator {

struct VulkanBackend::Impl_ {
  Impl_() : pDevice(nullptr), pInstance(nullptr), pSystemInfo(nullptr), hasValidSurface(false) { }
  ~Impl_() {
    if(pDevice.get() != nullptr) {
      vkb::destroy_device(*pDevice);
    }
    if(hasValidSurface) {
      vkb::destroy_surface(*pInstance, surface);
    }
    if(pInstance.get() != nullptr) {
      vkb::destroy_instance(*pInstance);
    }
  }

  void create_device(VulkanBackend &context) {
    // The app will likely crash if the window is not shown at this point, since this is typically
    // necessary in order to acquire a surface for Vulkan
    assert(context.window_.shown());
    context.window_.connect_to_graphics_api(
      IGraphicsBackend::GraphicsAPI::VULKAN, &(pInstance->instance), &surface);
    hasValidSurface = true;

    vkb::PhysicalDeviceSelector physicalDeviceSelector(*pInstance);

    // TODO: any other criteria we care about? Any required features/extensions?
    auto selection = physicalDeviceSelector.set_surface(surface).set_minimum_version(1, 1).select();
    validate_vkb_return(context, selection);

    vkb::DeviceBuilder deviceBuilder(selection.value());
    auto               deviceBuildResult = deviceBuilder.build();
    validate_vkb_return(context, deviceBuildResult);

    pDevice = std::make_unique<vkb::Device>(deviceBuildResult.value());
  }

  void create_instance(VulkanBackend &context) {
    vkb::InstanceBuilder instanceBuilder;

    instanceBuilder.set_app_name("Omulator").require_api_version(1, 1, 0);

    if(context.propertyMap_.get_prop<bool>(props::VKDEBUG).get()) {
      instanceBuilder.request_validation_layers();
      if(pSystemInfo->is_extension_available(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
        instanceBuilder.enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
          .set_debug_callback(&Impl_::debug_callback)
          .set_debug_callback_user_data_pointer(&context);
      }
    }

    auto instanceBuildResult = instanceBuilder.build();
    validate_vkb_return(context, instanceBuildResult);

    pInstance = std::make_unique<vkb::Instance>(instanceBuildResult.value());
  }

  static VkBool32 debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                 VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                 const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                 void                                       *pUserData) {
    auto      &context  = util::reinterpret<VulkanBackend>(pUserData);
    const auto severity = vkb::to_string_message_severity(messageSeverity);
    const auto type     = vkb::to_string_message_type(messageType);

    std::stringstream ss;
    ss << "Vulkan debug message [severity: " << severity << "; type: " << type
       << "] : " << pCallbackData->pMessage;
    context.logger_.warn(ss);

    // Must return this value per the standard
    return VK_FALSE;
  }

  void query_system_info(VulkanBackend &context) {
    auto systemInfoRet = vkb::SystemInfo::get_system_info();
    validate_vkb_return(context, systemInfoRet);
    pSystemInfo = std::make_unique<vkb::SystemInfo>(systemInfoRet.value());
  }

  template<typename T>
  // TODO: constrain me
  void validate_vkb_return(VulkanBackend &context, T &retval) {
    if(!retval) {
      std::stringstream ss;
      ss << "vk-bootstrap return failure for type "
         << util::TypeString<T> << "; reason: " << retval.error().message();
      vk_fatal(context, ss.str());
    }
  }

  void vk_fatal(VulkanBackend &context, const char *msg) {
    std::string errmsg = "Fatal Vulkan error: ";
    errmsg += msg;
    context.logger_.error(errmsg);
    throw std::runtime_error(errmsg);
  }

  void vk_fatal(VulkanBackend &context, const std::string &msg) { vk_fatal(context, msg.c_str()); }

  VkSurfaceKHR                     surface;
  std::unique_ptr<vkb::Device>     pDevice;
  std::unique_ptr<vkb::Instance>   pInstance;
  std::unique_ptr<vkb::SystemInfo> pSystemInfo;
  bool                             hasValidSurface;
};

VulkanBackend::VulkanBackend(ILogger &logger, PropertyMap &propertyMap, IWindow &window)
  : IGraphicsBackend(logger), propertyMap_(propertyMap), window_(window) {
  logger_.info("Initializing Vulkan, this may take a moment...");

  impl_->query_system_info(*this);
  impl_->create_instance(*this);
  impl_->create_device(*this);

  logger_.info("Vulkan initialization completed");
}

VulkanBackend::~VulkanBackend() { }

}  // namespace omulator