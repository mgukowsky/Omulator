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
    case Allocator::Residency::GPU_ONLY:
      return VMA_MEMORY_USAGE_GPU_ONLY;
    case Allocator::Residency::CPU_ONLY:
      return VMA_MEMORY_USAGE_CPU_ONLY;
    case Allocator::Residency::GPU_TO_CPU:
      return VMA_MEMORY_USAGE_GPU_TO_CPU;
    case Allocator::Residency::CPU_COPY:
      return VMA_MEMORY_USAGE_CPU_COPY;
    case Allocator::Residency::GPU_LAZILY_ALLOCATED:
      return VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
    case Allocator::Residency::AUTO:
      return VMA_MEMORY_USAGE_AUTO;
    case Allocator::Residency::AUTO_PREFER_DEVICE:
      return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    case Allocator::Residency::AUTO_PREFER_HOST:
      return VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
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

AllocatedImage Allocator::alloc(const vk::ImageCreateInfo &imageCreateInfo,
                                const Allocator::Residency residency) {
  VmaAllocationCreateInfo allocCreateInfo{
    .flags          = 0,
    .usage          = residency_to_vma_usage(residency),
    .requiredFlags  = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    .preferredFlags = 0,
    .memoryTypeBits = 0,
    .pool           = VK_NULL_HANDLE,
    .pUserData      = nullptr,
    .priority       = 0.0f};

  VkImage           image;
  VmaAllocation     allocation;
  VkImageCreateInfo rawImageCreateInfo(imageCreateInfo);

  validate_vk_return(
    logger_,
    "vmaCreateImage",
    vmaCreateImage(
      impl_->allocator, &rawImageCreateInfo, &allocCreateInfo, &image, &allocation, nullptr));

  return {image, allocation};
}

void Allocator::free(AllocatedBuffer &buff) {
  vmaDestroyBuffer(impl_->allocator, buff.handle, static_cast<VmaAllocation>(buff.pAllocation));
}

void Allocator::free(AllocatedImage &image) {
  vmaDestroyImage(impl_->allocator, image.handle, static_cast<VmaAllocation>(image.pAllocation));
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