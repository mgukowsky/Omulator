#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/util/Pimpl.hpp"
#include "omulator/vkmisc/vkmisc.hpp"

#include <vulkan/vulkan_raii.hpp>

namespace omulator::vkmisc {

/**
 * Class responsible for abstracting away logic related to sending data to and from the GPU.
 */
class Allocator {
public:
  /**
   * Used by the allocator to determine where to allocate the underlying memory.
   */
  enum class Residency {
    CPU_TO_GPU,
    GPU_ONLY,
    CPU_ONLY,
    GPU_TO_CPU,
    CPU_COPY,
    GPU_LAZILY_ALLOCATED,
    AUTO,
    AUTO_PREFER_DEVICE,
    AUTO_PREFER_HOST,
  };

  Allocator(ILogger                  &logger,
            vk::raii::Instance       &instance,
            vk::raii::PhysicalDevice &physicalDevice,
            vk::raii::Device         &device);
  ~Allocator();

  /**
   * Allocate memory on the GPU.
   */
  AllocatedBuffer alloc(const vk::BufferCreateInfo &bufferCreateInfo, const Residency residency);
  AllocatedImage  alloc(const vk::ImageCreateInfo &imageCreateInfo, const Residency residency);

  /**
   * Free memory allocated with alloc()
   */
  void free(AllocatedBuffer &buff);
  void free(AllocatedImage &image);

  /**
   * Update allocated memory on the GPU by copying over the contents of *localData.
   */
  void upload(AllocatedBuffer &buffer, void *localData, const std::size_t size);

private:
  struct Impl_;
  util::Pimpl<Impl_> impl_;

  ILogger &logger_;
};

}  // namespace omulator::vkmisc