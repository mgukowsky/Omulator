#include "omulator/vkmisc/Swapchain.hpp"

#include "omulator/IWindow.hpp"
#include "omulator/vkmisc/vkmisc.hpp"

#include <VkBootstrap.h>

#include <cassert>

namespace omulator::vkmisc {

struct Swapchain::Impl_ {
  vkb::Swapchain vkbSwapchain;
};

Swapchain::Swapchain(di::Injector     &injector,
                     ILogger          &logger,
                     IWindow          &window,
                     vk::raii::Device &device)
  : injector_(injector),
    logger_(logger),
    window_(window),
    device_(device),
    renderPass_(nullptr),
    swapchain_(nullptr),
    surfaceWidth(0),
    surfaceHeight(0) {
  // Almost the same as calling Swapchain::reset(), however we only have to create the renderpass
  // one time here
  // TODO: the above is true except in specific circumstances, like moving the window to and from an
  // HDR monitor... do we care?
  build_swapchain_();
  create_renderpass_();
  build_image_views_();
  build_framebuffers_();
}

Swapchain::~Swapchain() { }

std::pair<U32, U32> Swapchain::dimensions() const noexcept { return {surfaceWidth, surfaceHeight}; }

vk::raii::Framebuffer &Swapchain::framebuffer(const std::size_t idx) {
  assert(idx < framebuffers_.size() && "Framebuffer index must be in range");
  return framebuffers_.at(idx);
}

vk::Format Swapchain::image_format() {
  return static_cast<vk::Format>(impl_->vkbSwapchain.image_format);
}

vk::raii::RenderPass &Swapchain::renderPass() { return renderPass_; }

void Swapchain::reset() {
  clear_();
  build_swapchain_();
  build_image_views_();
  build_framebuffers_();
}

vk::raii::SwapchainKHR &Swapchain::swapchain() { return swapchain_; }

void Swapchain::build_framebuffers_() {
  for(const auto &imageView : imageViews_) {
    vk::FramebufferCreateInfo framebufferCreateInfo;
    framebufferCreateInfo.renderPass      = *renderPass_;
    framebufferCreateInfo.attachmentCount = 1;
    framebufferCreateInfo.width           = surfaceWidth;
    framebufferCreateInfo.height          = surfaceHeight;
    framebufferCreateInfo.layers          = 1;

    // TODO: depth attachment also needs to go in this array
    std::array attachments{*imageView};
    framebufferCreateInfo.pAttachments = attachments.data();

    framebuffers_.emplace_back(device_, framebufferCreateInfo);
  }
}

void Swapchain::build_image_views_() {
  for(const auto &image : swapchain_.getImages()) {
    vk::ImageViewCreateInfo createInfo;
    createInfo.image      = image;
    createInfo.viewType   = vk::ImageViewType::e2D;
    createInfo.format     = image_format();
    createInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY,
                             VK_COMPONENT_SWIZZLE_IDENTITY,
                             VK_COMPONENT_SWIZZLE_IDENTITY,
                             VK_COMPONENT_SWIZZLE_IDENTITY};

    createInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
    createInfo.subresourceRange.baseMipLevel   = 0;
    createInfo.subresourceRange.levelCount     = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount     = 1;

    imageViews_.emplace_back(device_, createInfo);
  }
}

void Swapchain::build_swapchain_() {
  auto                 &vkbDevice = injector_.get<vkb::Device>();
  vkb::SwapchainBuilder swapchainBuilder(vkbDevice);

  const auto dims = window_.dimensions();
  surfaceWidth    = dims.first;
  surfaceHeight   = dims.second;

  // TODO: we prefer triple buffering since it plays nicely with MAILBOX_KHR, but we should allow
  // for vsync w/ fewer buffers via FIFO_KHR, perhaps through a config value...
  swapchainBuilder
    .use_default_format_selection()
    // Can set to TRANSFER_DST_BIT if rendering somewhere else first
    .add_image_usage_flags(VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    // Don't blend with other windows in the window system
    .set_composite_alpha_flags(VkCompositeAlphaFlagBitsKHR::VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
    // Don't care about pixels that fall outside the presentable area (N.B. the docs advise
    // against fragment shaders with side-effects when using this setting)
    .set_clipped(true)
    .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
    .set_desired_extent(surfaceWidth, surfaceHeight)
    .set_desired_min_image_count(vkb::SwapchainBuilder::BufferMode::TRIPLE_BUFFERING);
  const auto swapchainBuildRet = swapchainBuilder.build();
  validate_vkb_return(logger_, swapchainBuildRet);
  impl_->vkbSwapchain = swapchainBuildRet.value();

  swapchain_ = vk::raii::SwapchainKHR(device_, impl_->vkbSwapchain.swapchain);
}

void Swapchain::clear_() {
  framebuffers_.clear();
  imageViews_.clear();
  swapchain_.clear();
}

void Swapchain::create_renderpass_() {
  // This is the description of the image our renderpass will target
  vk::AttachmentDescription colorAttachment;

  // Use the same format as the swapchain
  colorAttachment.format = image_format();

  // One sample; no MSAA
  colorAttachment.samples = vk::SampleCountFlagBits::e1;

  // Clear the values in the attachment to a constant when we start
  colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;

  // Renderpass contents will be stored in memory and can be read later
  colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;

  // Don't care about stencil ops
  colorAttachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
  colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

  // Don't care about the starting layout, but the final layout needs to be presentable by the
  // swapchain
  colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
  colorAttachment.finalLayout   = vk::ImageLayout::ePresentSrcKHR;

  // Describe the colors used by the subpass (the 0 is used to index into the attachments array in
  // the parent renderpass)
  vk::AttachmentReference colorAttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal);

  // Create our 1 minimally required graphics subpass for our renderpass
  vk::SubpassDescription subpassDescription;
  subpassDescription.pipelineBindPoint    = vk::PipelineBindPoint::eGraphics;
  subpassDescription.colorAttachmentCount = 1;
  subpassDescription.pColorAttachments    = &colorAttachmentReference;

  vk::SubpassDependency subpassDependency;
  // TODO: why does this not accept anything other than the macro?
  subpassDependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
  subpassDependency.dstSubpass    = 0;
  subpassDependency.srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  subpassDependency.srcAccessMask = vk::AccessFlagBits::eNone;
  subpassDependency.dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  subpassDependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

  // Create the actual renderpass
  vk::RenderPassCreateInfo renderPassCreateInfo;
  renderPassCreateInfo.attachmentCount = 1;
  renderPassCreateInfo.pAttachments    = &colorAttachment;
  renderPassCreateInfo.subpassCount    = 1;
  renderPassCreateInfo.pSubpasses      = &subpassDescription;
  renderPassCreateInfo.dependencyCount = 1;
  renderPassCreateInfo.pDependencies   = &subpassDependency;

  renderPass_ = vk::raii::RenderPass(device_, renderPassCreateInfo);
}

}  // namespace omulator::vkmisc
