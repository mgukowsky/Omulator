#include "omulator/vkmisc/Frame.hpp"

#include "omulator/vkmisc/VkBootstrapUtil.hpp"

namespace {

constexpr std::array    CLEAR_COLOR{0.0f, 0.0f, 0.0f, 1.0f};
constexpr omulator::U64 GPU_MAX_TIMEOUT_NS = 1'000'000'000;

}  // namespace

namespace omulator::vkmisc {
Frame::Frame(ILogger                 &logger,
             const std::size_t        id,
             vk::raii::Fence          fence,
             vk::raii::Semaphore      presentSemaphore,
             vk::raii::Semaphore      renderSemaphore,
             vk::raii::Device        &device,
             vk::raii::CommandBuffer &cmdBuff,
             Swapchain               &swapchain,
             Pipeline                &pipeline,
             DeviceQueues_t          &deviceQueues)
  : logger_(logger),
    id_(id),
    fence_(std::move(fence)),
    presentSemaphore_(std::move(presentSemaphore)),
    renderSemaphore_(std::move(renderSemaphore)),
    device_(device),
    cmdBuff_(cmdBuff),
    swapchain_(swapchain),
    pipeline_(pipeline),
    deviceQueues_(deviceQueues) { }

bool Frame::render() {
  cmdBuff_.reset();
  pipeline_.update_dynamic_state();

  auto [nextImgResult, imageIdx] =
    swapchain_.swapchain().acquireNextImage(GPU_MAX_TIMEOUT_NS, *presentSemaphore_);
  // On this codepath we should abandon the renderpass if we get eTimeout, and try again later. The
  // semaphore will remain unsignaled, but that's OK since we will try again at the next frame; as
  // long as the fences have been reset by this point we're OK to abandon the frame.
  if(nextImgResult == vk::Result::eTimeout || nextImgResult == vk::Result::eSuboptimalKHR
     || nextImgResult == vk::Result::eErrorOutOfDateKHR)
  {
    return false;
  }

  // Don't reset the fence until we're sure we will submit work to the GPU
  device_.resetFences({*fence_});

  recordBuff_(imageIdx);

  submit_();
  present_(imageIdx);

  return true;
}

bool Frame::wait() {
  const auto result = device_.waitForFences({*fence_}, VK_TRUE, GPU_MAX_TIMEOUT_NS);
  validate_vk_return(logger_, "waitForFences", result);
  // I _think_ we're good to continue if we get eTimeout here; in a situation where this times out
  // due to a breakpoint, we'll log the timeout, reset the fences, and continue on our merry way. I
  // guess it's _possible_ to get undefined behavior in other circumstances... I think? In any case,
  // we would get a timeout at this point every time we subsequently called this function if we
  // returned here in the face of eTimeout, since we would never again hit the code below to reset
  // the fences!

  // This error code means that the surface has been resized but VulkanBackend hasn't responded to
  // the resize yet
  if(result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR
     || result == vk::Result::eTimeout)
  {
    return false;
  }
  else {
    return true;
  }
}

void Frame::present_(const U32 imageIdx) {
  std::array         waitSemaphores{*renderSemaphore_};
  std::array         swapchains{*(swapchain_.swapchain())};
  vk::PresentInfoKHR presentInfo;
  presentInfo.swapchainCount     = static_cast<U32>(swapchains.size());
  presentInfo.pSwapchains        = swapchains.data();
  presentInfo.waitSemaphoreCount = static_cast<U32>(waitSemaphores.size());
  presentInfo.pWaitSemaphores    = waitSemaphores.data();
  presentInfo.pImageIndices      = &imageIdx;

  const auto result = deviceQueues_.at(QueueType_t::present).first.presentKHR(presentInfo);
  validate_vk_return(logger_, "presentKHR", result);
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

void Frame::submit_() {
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

  deviceQueues_.at(QueueType_t::graphics).first.submit(submitInfo, *fence_);
}

}  // namespace omulator::vkmisc
