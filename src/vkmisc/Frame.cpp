#include "omulator/vkmisc/Frame.hpp"

#include "omulator/vkmisc/vkmisc.hpp"

namespace {

constexpr std::array    CLEAR_COLOR{0.0f, 0.0f, 0.0f, 1.0f};
constexpr omulator::U64 GPU_MAX_TIMEOUT_NS = 1'000'000'000;

// Although ERROR_OUT_OF_DATE_KHR is recoverable, the spec considers it to be an error cofe for
// vkAcquireNextImageKHR and vkQueuePresentKHR. Given this, the generated vk:raii (correctly, per
// the spec) will throw if this error code is returned by either of these functions. To avoid
// wrapping each call in a try/catch block, we can instead call the underlying Vulkan function
// directly. Taken from
// https://github.com/KhronosGroup/Vulkan-Hpp/issues/599#issuecomment-1016468453
std::pair<vk::Result, uint32_t> rawAcquireNextImageKHR(const vk::raii::SwapchainKHR &swapchain,
                                                       uint64_t                      timeout,
                                                       vk::Semaphore                 semaphore,
                                                       vk::Fence                     fence) {
  uint32_t   image_index;
  vk::Result result = static_cast<vk::Result>(
    swapchain.getDispatcher()->vkAcquireNextImageKHR(static_cast<VkDevice>(swapchain.getDevice()),
                                                     static_cast<VkSwapchainKHR>(*swapchain),
                                                     timeout,
                                                     static_cast<VkSemaphore>(semaphore),
                                                     static_cast<VkFence>(fence),
                                                     &image_index));
  return std::make_pair(result, image_index);
}

vk::Result rawQueuePresentKHR(const vk::raii::Queue    &queue,
                              const vk::PresentInfoKHR &present_info) {
  return static_cast<vk::Result>(queue.getDispatcher()->vkQueuePresentKHR(
    static_cast<VkQueue>(*queue), reinterpret_cast<const VkPresentInfoKHR *>(&present_info)));
}

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

bool Frame::render(std::function<void(vk::raii::CommandBuffer &)> cmdfn) {
  cmdBuff_.reset();
  pipeline_.update_dynamic_state();

  auto [nextImgResult, imageIdx] =
    rawAcquireNextImageKHR(swapchain_.swapchain(), GPU_MAX_TIMEOUT_NS, *presentSemaphore_, nullptr);
  // N.B. that eTimeout can arise in certain legitimate situations, such as being paused on a
  // breakpoint.
  if(nextImgResult == vk::Result::eTimeout || nextImgResult == vk::Result::eSuboptimalKHR
     || nextImgResult == vk::Result::eErrorOutOfDateKHR)
  {
    return false;
  }

  // Don't reset the fence until we're sure we will submit work to the GPU
  device_.resetFences({*fence_});

  recordBuff_(imageIdx, cmdfn);

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

  const auto presentQueue = deviceQueues_.at(QueueType_t::present).first;
  validate_vk_return(logger_, "presentKHR", rawQueuePresentKHR(presentQueue, presentInfo));
}

void Frame::recordBuff_(const std::size_t                              idx,
                        std::function<void(vk::raii::CommandBuffer &)> cmdfn) {
  cmdBuff_.begin({});

  vk::RenderPassBeginInfo renderPassBeginInfo;
  renderPassBeginInfo.renderPass  = *(swapchain_.renderPass());
  renderPassBeginInfo.framebuffer = *(swapchain_.framebuffer(idx));

  const auto dims                = swapchain_.dimensions();
  renderPassBeginInfo.renderArea = vk::Rect2D({0, 0}, {dims.first, dims.second});

  vk::ClearValue clearValue;
  clearValue.color.setFloat32(CLEAR_COLOR);
  vk::ClearValue depthClear;
  depthClear.depthStencil.depth = 1.0f;

  std::array clearValues{clearValue, depthClear};

  renderPassBeginInfo.clearValueCount = static_cast<U32>(clearValues.size());
  renderPassBeginInfo.pClearValues    = clearValues.data();

  cmdBuff_.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
  cmdBuff_.bindPipeline(vk::PipelineBindPoint::eGraphics, *(pipeline_.pipeline()));

  cmdBuff_.setViewport(0, pipeline_.viewport());
  cmdBuff_.setScissor(0, pipeline_.scissor());

  cmdfn(cmdBuff_);

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
