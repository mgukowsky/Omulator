#include "omulator/vkmisc/Initializer.hpp"

#include "omulator/VulkanBackend.hpp"
#include "omulator/props.hpp"
#include "omulator/util/reinterpret.hpp"
#include "omulator/vkmisc/Allocator.hpp"
#include "omulator/vkmisc/Pipeline.hpp"
#include "omulator/vkmisc/Swapchain.hpp"
#include "omulator/vkmisc/vkmisc.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <VkBootstrap.h>

#include <array>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace omulator::vkmisc {

vk::raii::CommandBuffers *command_buffers_recipe(di::Injector &injector) {
  auto &commandPool = injector.get<vk::raii::CommandPool>();

  vk::CommandBufferAllocateInfo commandBufferAllocateInfo(
    *commandPool, vk::CommandBufferLevel::ePrimary, NUM_FRAMES_IN_FLIGHT);

  return new vk::raii::CommandBuffers(injector.get<vk::raii::Device>(), commandBufferAllocateInfo);
}

vk::raii::CommandPool *command_pool_recipe(di::Injector &injector) {
  auto &deviceQueues = injector.get<DeviceQueues_t>();

  // At this point we are past what vk-bootstrap can offer us, so we turn to vulkan.hpp for
  // additional convenience and type safety
  vk::CommandPoolCreateInfo commandPoolCreateInfo(
    vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
    deviceQueues.at(QueueType_t::graphics).second);

  return new vk::raii::CommandPool(injector.get<vk::raii::Device>(), commandPoolCreateInfo);
}

/**
 * Custom callback which will be invoked by the Vulkan implementation.
 */
VkBool32 debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                        VkDebugUtilsMessageTypeFlagsEXT             messageType,
                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                        void                                       *pUserData) {
  auto      &logger   = util::reinterpret<ILogger>(pUserData);
  const auto severity = vkb::to_string_message_severity(messageSeverity);
  const auto type     = vkb::to_string_message_type(messageType);

  std::stringstream ss;
  ss << "Vulkan debug message [severity: " << severity << "; type: " << type
     << "] : " << pCallbackData->pMessage;
  logger.warn(ss);

  // Must return this value per the standard
  return VK_FALSE;
}

vk::raii::DebugUtilsMessengerEXT *debug_utils_messenger_recipe(di::Injector &injector) {
  return new vk::raii::DebugUtilsMessengerEXT(injector.get<vk::raii::Instance>(),
                                              injector.get<vkb::Instance>().debug_messenger);
}

vk::raii::Device *device_recipe(di::Injector &injector) {
  auto *pDevice = new vk::raii::Device(injector.get<vk::raii::PhysicalDevice>(),
                                       injector.get<vkb::Device>().device);

  // Part 3 of vulkan.hpp's initialization process, per
  // https://github.com/KhronosGroup/Vulkan-Hpp#extensions--per-device-function-pointers
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*(*pDevice));

  return pDevice;
}

DeviceQueues_t *device_queues_recipe(di::Injector &injector) {
  // We will retrieve a queue from the device for each of these types, taking care to map the
  // vk-bootstrap enum to our publicly visible enum
  constexpr std::array QUEUE_TYPES{
    std::pair{vkb::QueueType::graphics, QueueType_t::graphics},
    std::pair{vkb::QueueType::present,  QueueType_t::present }
  };

  auto &logger    = injector.get<ILogger>();
  auto &vkbDevice = injector.get<vkb::Device>();

  auto *deviceQueues = new DeviceQueues_t;

  for(const auto &queueType : QUEUE_TYPES) {
    auto queueRet = vkbDevice.get_queue(queueType.first);
    validate_vkb_return(logger, queueRet);

    auto indexRet = vkbDevice.get_queue_index(queueType.first);
    validate_vkb_return(logger, indexRet);
    const U32 queueFamily = indexRet.value();

    deviceQueues->emplace(
      queueType.second,
      std::pair<vk::raii::Queue, const U32>{
        vk::raii::Queue(injector.get<vk::raii::Device>(), queueRet.value()), queueFamily});
  }

  return deviceQueues;
}

vk::raii::Fence *fence_recipe(di::Injector &injector) {
  vk::FenceCreateInfo fenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);
  return new vk::raii::Fence(injector.get<vk::raii::Device>(), fenceCreateInfo);
}

vk::raii::Instance *instance_recipe(di::Injector &injector) {
  return new vk::raii::Instance(injector.get<vk::raii::Context>(),
                                injector.get<vkb::Instance>().instance);
}

vk::raii::PhysicalDevice *physical_device_recipe(di::Injector &injector) {
  return new vk::raii::PhysicalDevice(injector.get<vk::raii::Instance>(),
                                      injector.get<vkb::Device>().physical_device.physical_device);
}

vk::raii::Semaphore *semaphore_recipe(di::Injector &injector) {
  vk::SemaphoreCreateInfo semaphoreCreateInfo;
  return new vk::raii::Semaphore(injector.get<vk::raii::Device>(), semaphoreCreateInfo);
}

vk::raii::SurfaceKHR *surface_recipe(di::Injector &injector) {
  auto &instance = injector.get<vk::raii::Instance>();
  return new vk::raii::SurfaceKHR(
    instance,
    reinterpret_cast<VkSurfaceKHR>(injector.get<IWindow>().connect_to_graphics_api(
      IGraphicsBackend::GraphicsAPI::VULKAN, &instance)));
}

vkb::SystemInfo *system_info_recipe(di::Injector &injector) {
  auto &logger = injector.get<ILogger>();

  auto systemInfoRet = vkb::SystemInfo::get_system_info();
  validate_vkb_return(logger, systemInfoRet);
  auto *pSystemInfo = new vkb::SystemInfo(systemInfoRet.value());

  std::stringstream ss;
  ss << "Instance info: \n";
  ss << "Available extensions: ";

  for(const auto &ext : pSystemInfo->available_extensions) {
    ss << "\t" << ext.extensionName << "(" << ext.specVersion << ")\n";
  }

  ss << "\nAvailable layers: ";
  for(const auto &layer : pSystemInfo->available_layers) {
    ss << "\t" << layer.layerName << "(spec: " << layer.specVersion
       << "; impl: " << layer.implementationVersion << " ): " << layer.description << "\n";
  }

  logger.debug(ss);

  return pSystemInfo;
}

vkb::Device *vkb_device_recipe(di::Injector &injector) {
  auto &logger      = injector.get<ILogger>();
  auto &propertyMap = injector.get<PropertyMap>();

  // The point of this block is to ensure that the debug utils are created after the
  // instance but before the device. vk-bootstrap creates the debug utils under the hood (if
  // needed), but we need to make sure the raii instance is created in the right place so that it
  // is subsequently destroyed at the right time.
  if(propertyMap.get_prop<bool>(props::VKDEBUG).get()) {
    std::stringstream ss;
    ss << "DebugUtilsMessengerEXT created at "
       << &(injector.get<vk::raii::DebugUtilsMessengerEXT>());
    logger.trace(ss);
  }

  vkb::PhysicalDeviceSelector physicalDeviceSelector(injector.get<vkb::Instance>());

  // The app will likely crash if the window is not shown at this point, since this is typically
  // necessary in order to acquire a surface for Vulkan
  assert(injector.get<IWindow>().shown());

  auto &surface = injector.get<vk::raii::SurfaceKHR>();

  // TODO: any other criteria we care about? Any required features/extensions?
  auto selection = physicalDeviceSelector.set_surface(*surface)
                     .set_minimum_version(1, 1)
                     .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
                     .require_present(true)
                     .add_required_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
                     .select();
  validate_vkb_return(logger, selection);

  {
    std::stringstream ss;
    ss << "Selected physical Vulkan device: " << selection.value().name;
    logger.info(ss);
  }

  vkb::DeviceBuilder deviceBuilder(selection.value());
  const auto         deviceBuildResult = deviceBuilder.build();
  validate_vkb_return(logger, deviceBuildResult);

  return new vkb::Device(deviceBuildResult.value());
}

vkb::Instance *vkb_instance_recipe(di::Injector &injector) {
  auto &logger      = injector.get<ILogger>();
  auto &propertyMap = injector.get<PropertyMap>();
  auto &systemInfo  = injector.get<vkb::SystemInfo>();

  vkb::InstanceBuilder instanceBuilder;

  instanceBuilder.set_app_name("Omulator").require_api_version(OML_VK_VERSION);

  // Enable enumeration of non-conformant implementations which may be installed. See
  // https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Instance#page_Encountered-VK_ERROR_INCOMPATIBLE_DRIVER
  if(systemInfo.is_extension_available(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
    instanceBuilder.enable_extension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  }

  if(propertyMap.get_prop<bool>(props::VKDEBUG).get()) {
    instanceBuilder.request_validation_layers();
    if(systemInfo.debug_utils_available) {
      instanceBuilder.enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
        .set_debug_callback(&debug_callback)
        .set_debug_callback_user_data_pointer(&logger);
    }
  }

  auto instanceBuildResult = instanceBuilder.build();
  validate_vkb_return(logger, instanceBuildResult);

  auto *pVkbInstance = new vkb::Instance(instanceBuildResult.value());

  // Parts 1 and 2 of vulkan.hpp's initialization process, per
  // https://github.com/KhronosGroup/Vulkan-Hpp#extensions--per-device-function-pointers
  //
  // Also recommended for interoperatiblity with vk-bootstrap per//
  // https://github.com/charles-lunarg/vk-bootstrap/issues/68
  VULKAN_HPP_DEFAULT_DISPATCHER.init(pVkbInstance->fp_vkGetInstanceProcAddr);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(pVkbInstance->instance);

  return pVkbInstance;
}

void install_vk_initializer_rules(di::Injector &injector) {
  // Custom recipes
  injector.addRecipe<vkb::SystemInfo>(&system_info_recipe);
  injector.addRecipe<vkb::Instance>(&vkb_instance_recipe);
  injector.addRecipe<vk::raii::Instance>(&instance_recipe);
  injector.addRecipe<vk::raii::DebugUtilsMessengerEXT>(&debug_utils_messenger_recipe);
  injector.addRecipe<vk::raii::SurfaceKHR>(&surface_recipe);
  injector.addRecipe<vkb::Device>(&vkb_device_recipe);
  injector.addRecipe<vk::raii::PhysicalDevice>(&physical_device_recipe);
  injector.addRecipe<vk::raii::Device>(&device_recipe);
  injector.addRecipe<DeviceQueues_t>(&device_queues_recipe);
  injector.addRecipe<vk::raii::CommandPool>(&command_pool_recipe);
  injector.addRecipe<vk::raii::CommandBuffers>(&command_buffers_recipe);
  injector.addRecipe<vk::raii::Fence>(&fence_recipe);
  injector.addRecipe<vk::raii::Semaphore>(&semaphore_recipe);

  // ctor recipes
  injector.addCtorRecipe<vkmisc::Allocator,
                         ILogger &,
                         vk::raii::Instance &,
                         vk::raii::PhysicalDevice &,
                         vk::raii::Device &>();

  injector.addCtorRecipe<vkmisc::Swapchain,
                         di::Injector &,
                         ILogger &,
                         IWindow &,
                         vkmisc::Allocator &,
                         vk::raii::Device &>();
  injector.addCtorRecipe<vkmisc::Pipeline,
                         ILogger &,
                         vk::raii::Device &,
                         vkmisc::Swapchain &,
                         PropertyMap &>();

  injector.addCtorRecipe<VulkanBackend,
                         ILogger &,
                         di::Injector &,
                         IWindow &,
                         vk::raii::Device &,
                         vkmisc::Swapchain &,
                         vkmisc::Pipeline &,
                         vkmisc::DeviceQueues_t &,
                         vkmisc::Allocator &>();

  // Interfaces bound to implementations
  injector.bindImpl<IGraphicsBackend, VulkanBackend>();
}

}  // namespace omulator::vkmisc