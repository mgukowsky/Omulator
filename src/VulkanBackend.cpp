#include "omulator/VulkanBackend.hpp"

#include "omulator/vkmisc/Initializer.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <cassert>
#include <utility>

namespace omulator {

VulkanBackend::VulkanBackend(ILogger                &logger,
                             di::Injector           &injector,
                             IWindow                &window,
                             vk::raii::Device       &device,
                             vkmisc::Swapchain      &swapchain,
                             vkmisc::Pipeline       &pipeline,
                             vkmisc::DeviceQueues_t &deviceQueues)
  : IGraphicsBackend(logger, injector),
    window_(window),
    device_(device),
    swapchain_(swapchain),
    pipeline_(pipeline) {
  logger_.info("Initializing Vulkan, this may take a moment...");

  swapchain_.reset();

  for(std::size_t i = 0; i < vkmisc::NUM_FRAMES_IN_FLIGHT; ++i) {
    auto &cmdBuffs = injector.get<vk::raii::CommandBuffers>();
    assert(i < cmdBuffs.size()
           && "At least NUM_FRAMES_IN_FLIGHT command buffers need to be created");

    frames_.emplace_back(i,
                         injector.creat_move<vk::raii::Fence>(),
                         injector.creat_move<vk::raii::Semaphore>(),
                         injector.creat_move<vk::raii::Semaphore>(),
                         device_,
                         cmdBuffs.at(i),
                         swapchain_,
                         pipeline_,
                         deviceQueues);
  }

  logger_.info("Vulkan initialization completed");
}
VulkanBackend::~VulkanBackend() { device_.waitIdle(); }

void VulkanBackend::handle_resize() {
  const auto windowDims = window_.dimensions();
  // The window is minimized or otherwise not visible; don't resize and don't render
  if(windowDims.first <= 0 || windowDims.second <= 0) {
    shouldRender_ = false;
  }
  else {
    shouldRender_  = true;
    needsResizing_ = true;
  }
}

void VulkanBackend::render_frame() { frames_[0].render(); }

void VulkanBackend::do_resize_() { swapchain_.reset(); }
}  // namespace omulator
