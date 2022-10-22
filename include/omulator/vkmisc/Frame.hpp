#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace omulator::vkmisc {

/**
 * Class responsible for synchronzation with and submitting units of work to the GPU.
 */
class Frame {
public:
  Frame(const std::size_t        id,
        vk::raii::Fence          fence,
        vk::raii::Semaphore      presentSemaphore,
        vk::raii::Semaphore      renderSemaphore,
        vk::raii::CommandBuffer &cmdBuff);

private:
  const std::size_t id_;

  vk::raii::Fence     fence_;
  vk::raii::Semaphore presentSemaphore_;
  vk::raii::Semaphore renderSemaphore_;

  vk::raii::CommandBuffer &cmdBuff_;
};

}  // namespace omulator::vkmisc