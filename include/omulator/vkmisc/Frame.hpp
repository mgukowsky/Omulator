#pragma once

#include "omulator/vkmisc/Initializer.hpp"
#include "omulator/vkmisc/Pipeline.hpp"
#include "omulator/vkmisc/Swapchain.hpp"

#include <vulkan/vulkan_raii.hpp>

namespace omulator::vkmisc {

/**
 * Class responsible for synchronzation with and submitting units of work to the GPU.
 */
class Frame {
public:
  Frame(ILogger                 &logger,
        const std::size_t        id,
        vk::raii::Fence          fence,
        vk::raii::Semaphore      presentSemaphore,
        vk::raii::Semaphore      renderSemaphore,
        vk::raii::Device        &device,
        vk::raii::CommandBuffer &cmdBuff,
        Swapchain               &swapchain,
        Pipeline                &pipeline,
        DeviceQueues_t          &deviceQueues);

  /**
   * Send a draw call to the GPU. Returns true if the frame was successfully submitted.
   */
  bool render();

  /**
   * Wait for any work this frame has submitted to the GPU to complete. Returns true if the wait
   * operation complete successfully.
   */
  bool wait();

private:
  void recordBuff_(const std::size_t idx);

  ILogger          &logger_;
  const std::size_t id_;

  vk::raii::Fence     fence_;
  vk::raii::Semaphore presentSemaphore_;
  vk::raii::Semaphore renderSemaphore_;

  vk::raii::Device        &device_;
  vk::raii::CommandBuffer &cmdBuff_;
  Swapchain               &swapchain_;
  Pipeline                &pipeline_;
  DeviceQueues_t          &deviceQueues_;
};

}  // namespace omulator::vkmisc
