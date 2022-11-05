#include "omulator/vkmisc/Allocator.hpp"

#include "omulator/vkmisc/vkmisc.hpp"

#include <cstring>
#include <stdexcept>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

using omulator::vkmisc::Allocator;

namespace {
VmaMemoryUsage residency_to_vma_usage(const Allocator::Residency residency) {
  switch(residency) {
    case Allocator::Residency::CPU_TO_GPU:
      return VMA_MEMORY_USAGE_CPU_TO_GPU;
    default:
      throw std::runtime_error("Invalid Residency enum value!");
  }
}
}  // namespace

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

Allocator::~Allocator() { vmaDestroyAllocator(impl_->allocator); }

AllocatedBuffer Allocator::alloc(const vk::BufferCreateInfo &bufferCreateInfo,
                                 const Allocator::Residency  residency) {
  VmaAllocationCreateInfo allocCreateInfo{.flags          = 0,
                                          .usage          = residency_to_vma_usage(residency),
                                          .requiredFlags  = 0,
                                          .preferredFlags = 0,
                                          .memoryTypeBits = 0,
                                          .pool           = VK_NULL_HANDLE,
                                          .pUserData      = nullptr,
                                          .priority       = 0.0f};

  VkBuffer           buff;
  VmaAllocation      allocation;
  VkBufferCreateInfo rawBufferCreateInfo(bufferCreateInfo);

  validate_vk_return(
    logger_,
    "vmaCreateBuffer",
    vmaCreateBuffer(
      impl_->allocator, &(rawBufferCreateInfo), &allocCreateInfo, &buff, &allocation, nullptr));

  return {buff, allocation};
}

void Allocator::free(AllocatedBuffer &buff) {
  vmaDestroyBuffer(impl_->allocator, buff.buffer, static_cast<VmaAllocation>(buff.pAllocation));
}

void Allocator::upload(AllocatedBuffer &buffer, void *localData, const std::size_t size) {
  void *gpuData;
  validate_vk_return(
    logger_,
    "vmaMapMemory",
    vmaMapMemory(impl_->allocator, static_cast<VmaAllocation>(buffer.pAllocation), &gpuData));

  std::memcpy(gpuData, localData, size);

  // TODO: for frequently updated data it is inefficient to map and unmap the data on each update;
  // add a streaming API to map the memory once (if the need arises)
  vmaUnmapMemory(impl_->allocator, static_cast<VmaAllocation>(buffer.pAllocation));
}

}  // namespace omulator::vkmisc