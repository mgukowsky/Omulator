#include "omulator/vkmisc/Frame.hpp"

namespace {

constexpr std::array    CLEAR_COLOR{0.0f, 1.0f, 0.0f, 1.0f};
constexpr omulator::U64 GPU_MAX_TIMEOUT_NS = 5'000'000'000;

}  // namespace

namespace omulator::vkmisc {
Frame::Frame(const std::size_t        id,
             vk::raii::Fence          fence,
             vk::raii::Semaphore      presentSemaphore,
             vk::raii::Semaphore      renderSemaphore,
             vk::raii::Device        &device,
             vk::raii::CommandBuffer &cmdBuff,
             Swapchain               &swapchain,
             Pipeline                &pipeline,
             DeviceQueues_t          &deviceQueues)
  : id_(id),
    fence_(std::move(fence)),
    presentSemaphore_(std::move(presentSemaphore)),
    renderSemaphore_(std::move(renderSemaphore)),
    device_(device),
    cmdBuff_(cmdBuff),
    swapchain_(swapchain),
    pipeline_(pipeline),
    deviceQueues_(deviceQueues) { }

void Frame::render() {
  // TODO: error handling
  if(device_.waitForFences({*fence_}, VK_TRUE, GPU_MAX_TIMEOUT_NS) != vk::Result::eSuccess) {
    throw std::runtime_error(":(");
  }
  device_.resetFences({*fence_});

  // TODO: resize logic needs to happen here?

  cmdBuff_.reset();

  // TODO: error handling
  auto [result, imageIdx] =
    swapchain_.swapchain().acquireNextImage(GPU_MAX_TIMEOUT_NS, *presentSemaphore_);

  recordBuff_(imageIdx);

  {
    std::array             waitSemaphores{*presentSemaphore_};
    std::array             signalSemaphores{*renderSemaphore_};
    std::array             cmdBuffs{*cmdBuff_};
    vk::PipelineStageFlags waitDstStageMask{vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::SubmitInfo         submitInfo;
    submitInfo.waitSemaphoreCount   = static_cast<U32>(waitSemaphores.size());
    submitInfo.pWaitSemaphores      = waitSemaphores.data();
    submitInfo.signalSemaphoreCount = static_cast<U32>(signalSemaphores.size());
    submitInfo.pSignalSemaphores    = signalSemaphores.data();
    submitInfo.pWaitDstStageMask    = &waitDstStageMask;
    submitInfo.commandBufferCount   = static_cast<U32>(cmdBuffs.size());
    submitInfo.pCommandBuffers      = cmdBuffs.data();

    // TODO: error handling
    deviceQueues_.at(QueueType_t::graphics).first.submit(submitInfo, *fence_);
  }

  {
    std::array         waitSemaphores{*renderSemaphore_};
    std::array         swapchains{*(swapchain_.swapchain())};
    vk::PresentInfoKHR presentInfo;
    presentInfo.swapchainCount     = static_cast<U32>(swapchains.size());
    presentInfo.pSwapchains        = swapchains.data();
    presentInfo.waitSemaphoreCount = static_cast<U32>(waitSemaphores.size());
    presentInfo.pWaitSemaphores    = waitSemaphores.data();
    presentInfo.pImageIndices      = &imageIdx;

    // TODO: error handling
    if(deviceQueues_.at(QueueType_t::present).first.presentKHR(presentInfo) != vk::Result::eSuccess)
    {
      throw std::runtime_error(":(");
    }
  }
}

void Frame::recordBuff_(const std::size_t idx) {
  cmdBuff_.begin({});

  vk::RenderPassBeginInfo renderPassBeginInfo;
  renderPassBeginInfo.renderPass  = *(swapchain_.renderPass());
  renderPassBeginInfo.framebuffer = *(swapchain_.framebuffer(idx));

  const auto dims                = swapchain_.dimensions();
  renderPassBeginInfo.renderArea = vk::Rect2D({0, 0}, {dims.first, dims.second});

  vk::ClearValue clearValue;
  clearValue.color.setFloat32(CLEAR_COLOR);
  renderPassBeginInfo.clearValueCount = 1;
  renderPassBeginInfo.pClearValues    = &clearValue;

  cmdBuff_.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
  cmdBuff_.bindPipeline(vk::PipelineBindPoint::eGraphics, *(pipeline_.pipeline()));
  cmdBuff_.setViewport(0, pipeline_.viewport());
  cmdBuff_.setScissor(0, pipeline_.scissor());

  cmdBuff_.draw(3, 1, 0, 0);

  cmdBuff_.endRenderPass();
  cmdBuff_.end();
}
}  // namespace omulator::vkmisc
