#include "omulator/vkmisc/Allocator.hpp"

#include "omulator/vkmisc/VkBootstrapUtil.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace omulator::vkmisc {

struct Allocator::Impl_ {
  VmaAllocator allocator;
};

Allocator::Allocator(ILogger                  &logger,
                     vk::raii::Instance       &instance,
                     vk::raii::PhysicalDevice &physicalDevice,
                     vk::raii::Device         &device)
  : logger_(logger) {
  VmaAllocatorCreateInfo createInfo = {
    .flags                          = {},
    .physicalDevice                 = *physicalDevice,
    .device                         = *device,
    .preferredLargeHeapBlockSize    = 0,
    .pAllocationCallbacks           = nullptr,
    .pDeviceMemoryCallbacks         = nullptr,
    .pHeapSizeLimit                 = nullptr,
    .pVulkanFunctions               = nullptr,
    .instance                       = *instance,
    .vulkanApiVersion               = OML_VK_VERSION,
    .pTypeExternalMemoryHandleTypes = nullptr,
  };

  validate_vk_return(
    logger, "vmaCreateAllocator", vmaCreateAllocator(&createInfo, &(impl_->allocator)));
}

Allocator::~Allocator() { }

}  // namespace omulator::vkmisc