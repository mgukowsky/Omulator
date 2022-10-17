#include "omulator/VulkanBackend.hpp"

#include "omulator/oml_types.hpp"
#include "omulator/props.hpp"
#include "omulator/util/TypeString.hpp"
#include "omulator/util/reinterpret.hpp"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <VkBootstrap.h>

#include <cassert>
#include <functional>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace {

/**
 * VkResult to string; courtesy of
 * https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanTools.cpp
 */
std::string_view vkErrorString(const VkResult errorCode) {
  switch(errorCode) {
#define VKSTRCONV(r) \
  case VK_##r:       \
    return #r
    VKSTRCONV(NOT_READY);
    VKSTRCONV(TIMEOUT);
    VKSTRCONV(EVENT_SET);
    VKSTRCONV(EVENT_RESET);
    VKSTRCONV(INCOMPLETE);
    VKSTRCONV(ERROR_OUT_OF_HOST_MEMORY);
    VKSTRCONV(ERROR_OUT_OF_DEVICE_MEMORY);
    VKSTRCONV(ERROR_INITIALIZATION_FAILED);
    VKSTRCONV(ERROR_DEVICE_LOST);
    VKSTRCONV(ERROR_MEMORY_MAP_FAILED);
    VKSTRCONV(ERROR_LAYER_NOT_PRESENT);
    VKSTRCONV(ERROR_EXTENSION_NOT_PRESENT);
    VKSTRCONV(ERROR_FEATURE_NOT_PRESENT);
    VKSTRCONV(ERROR_INCOMPATIBLE_DRIVER);
    VKSTRCONV(ERROR_TOO_MANY_OBJECTS);
    VKSTRCONV(ERROR_FORMAT_NOT_SUPPORTED);
    VKSTRCONV(ERROR_SURFACE_LOST_KHR);
    VKSTRCONV(ERROR_NATIVE_WINDOW_IN_USE_KHR);
    VKSTRCONV(SUBOPTIMAL_KHR);
    VKSTRCONV(ERROR_OUT_OF_DATE_KHR);
    VKSTRCONV(ERROR_INCOMPATIBLE_DISPLAY_KHR);
    VKSTRCONV(ERROR_VALIDATION_FAILED_EXT);
    VKSTRCONV(ERROR_INVALID_SHADER_NV);
#undef VKSTRCONV
    default:
      return "UNKNOWN_ERROR";
  }
}

/**
 * Amount of time after which a timeout in the GPU will be considered a fatal error, in nanoseconds.
 */
constexpr omulator::U64 GPU_MAX_TIMEOUT_NS = 1'000'000'000;

}  // namespace

namespace omulator {

using AgnosticDeleter_t = std::function<void()>;

struct VulkanBackend::Impl_ {
  /**
   * Create a C++ Vulkan object and record its order of deletion into the FIFO.
   */
  template<typename T>
  // TODO: constrain me
  void init_with_deleter(VulkanBackend &context, T &dst, std::function<T()> initializer) {
    dst = initializer();
    deleterFIFO.emplace_back([&]() {
      std::string msg = "Deleting Vulkan object of type: ";
      msg += util::TypeString<T>;
      context.logger_.debug(msg);
      dst.reset(nullptr);
    });
  }

  /**
   * Version which works with ranged-for loop supporting containers which contain Vulkan objects.
   */
  template<typename T>
  // TODO: constrain me
  void init_with_deleter_range(VulkanBackend &context, T &dst, std::function<T()> initializer) {
    dst = initializer();
    deleterFIFO.emplace_back([&]() {
      std::string msg = "Deleting multiple Vulkan objects of type: ";
      msg += util::TypeString<T>;
      context.logger_.debug(msg);
      for(auto &elem : dst) {
        elem.reset(nullptr);
      }
    });
  }

  Impl_()
    : mainCommandBuffers{},
      commandPool(nullptr),
      renderPass(nullptr),
      graphicsQueue(nullptr),
      graphicsQueueFamily(0),
      surface(nullptr),
      pDevice(nullptr),
      pInstance(nullptr),
      pSwapchain(nullptr),
      pSystemInfo(nullptr),
      numFrames(0) { }
  ~Impl_() {
    device.waitIdle();

    // Clear out the FIFO; from https://vkguide.dev/docs/chapter-2/cleanup/
    for(auto it = deleterFIFO.rbegin(); it != deleterFIFO.rend(); ++it) {
      (*it)();
    }
  }

  /**
   * Custom callback which will be invoked by the Vulkan implementation.
   */
  static VkBool32 debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                 VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                 const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                 void                                       *pUserData) {
    auto      &context  = util::reinterpret<VulkanBackend>(pUserData);
    const auto severity = vkb::to_string_message_severity(messageSeverity);
    const auto type     = vkb::to_string_message_type(messageType);

    std::stringstream ss;
    ss << "Vulkan debug message [severity: " << severity << "; type: " << type
       << "] : " << pCallbackData->pMessage;
    context.logger_.warn(ss);

    // Must return this value per the standard
    return VK_FALSE;
  }

  /**
   * We lay out a series of functions here which are used to intialize Vulkan. They are intended to
   * be invoked in a specific order (and will likely cause undefined behavior if called out of
   * order), so to aid in correct usage we prefix each function with a step indicating the order in
   * which the functions should be invoked.
   */
  void step1_query_system_info(VulkanBackend &context) {
    auto systemInfoRet = vkb::SystemInfo::get_system_info();
    validate_vkb_return(context, systemInfoRet);
    pSystemInfo = std::make_unique<vkb::SystemInfo>(systemInfoRet.value());
  }

  void step2_create_instance(VulkanBackend &context) {
    vkb::InstanceBuilder instanceBuilder;

    instanceBuilder.set_app_name("Omulator").require_api_version(1, 1, 0);

    if(context.propertyMap_.get_prop<bool>(props::VKDEBUG).get()) {
      instanceBuilder.request_validation_layers();
      if(pSystemInfo->is_extension_available(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
        instanceBuilder.enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
          .set_debug_callback(&Impl_::debug_callback)
          .set_debug_callback_user_data_pointer(&context);
      }
    }

    auto instanceBuildResult = instanceBuilder.build();
    validate_vkb_return(context, instanceBuildResult);

    pInstance = std::make_unique<vkb::Instance>(instanceBuildResult.value());
    deleterFIFO.emplace_back([this]() { vkb::destroy_instance(*pInstance); });

    // Parts 1 and 2 of vulkan.hpp's initialization process, per
    // https://github.com/KhronosGroup/Vulkan-Hpp#extensions--per-device-function-pointers
    //
    // Also recommended for interoperatiblity with vk-bootstrap per//
    // https://github.com/charles-lunarg/vk-bootstrap/issues/68
    VULKAN_HPP_DEFAULT_DISPATCHER.init(pInstance->fp_vkGetInstanceProcAddr);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(pInstance->instance);
  }

  void step3_create_device(VulkanBackend &context) {
    // The app will likely crash if the window is not shown at this point, since this is typically
    // necessary in order to acquire a surface for Vulkan
    assert(context.window_.shown());
    deleterFIFO.emplace_back([this]() { vkb::destroy_surface(*pInstance, surface); });

    context.window_.connect_to_graphics_api(
      IGraphicsBackend::GraphicsAPI::VULKAN, &(pInstance->instance), &surface);

    vkb::PhysicalDeviceSelector physicalDeviceSelector(*pInstance);

    // TODO: any other criteria we care about? Any required features/extensions?
    auto selection = physicalDeviceSelector.set_surface(surface).set_minimum_version(1, 1).select();
    validate_vkb_return(context, selection);

    vkb::DeviceBuilder deviceBuilder(selection.value());
    const auto         deviceBuildResult = deviceBuilder.build();
    validate_vkb_return(context, deviceBuildResult);

    pDevice = std::make_unique<vkb::Device>(deviceBuildResult.value());
    deleterFIFO.emplace_back([this]() { vkb::destroy_device(*pDevice); });
    device = *pDevice;

    // Part 3 of vulkan.hpp's initialization process, per
    // https://github.com/KhronosGroup/Vulkan-Hpp#extensions--per-device-function-pointers
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
  }

  void step4_get_queues(VulkanBackend &context) {
    auto graphicsQueueRet = pDevice->get_queue(vkb::QueueType::graphics);
    validate_vkb_return(context, graphicsQueueRet);

    graphicsQueue = vk::Queue(graphicsQueueRet.value());

    auto graphicsQueueIndexRet = pDevice->get_queue_index(vkb::QueueType::graphics);
    validate_vkb_return(context, graphicsQueueIndexRet);
    graphicsQueueFamily = graphicsQueueIndexRet.value();

    // TODO: any other queues we want to retrieve?
  }

  void step5_create_swapchain(VulkanBackend &context) {
    vkb::SwapchainBuilder swapchainBuilder(*pDevice);
    // TODO: VK_PRESENT_MODE_FIFO_KHR is a vsync mode; we want to use VK_PRESENT_MODE_MAILBOX_KHR
    // + triple buffering when not using vsync (there is also another present mode with 'softer'
    // vsync); at the very least this should be tied to a config setting
    //
    // TODO: We will need to destroy and rebuild the swapchain when the window resizes!
    const auto dims = context.window_.dimensions();

    swapchainBuilder.use_default_format_selection()
      .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
      .set_desired_extent(dims.first, dims.second);
    const auto swapchainBuildRet = swapchainBuilder.build();
    validate_vkb_return(context, swapchainBuildRet);
    pSwapchain = std::make_unique<vkb::Swapchain>(swapchainBuildRet.value());
    deleterFIFO.emplace_back([this]() { vkb::destroy_swapchain(*pSwapchain); });

    swapchain = *pSwapchain;
  }

  void step6_create_commands([[maybe_unused]] VulkanBackend &context) {
    // At this point we are past what vk-bootstrap can offer us, so we turn to vulkan.hpp for
    // additional convenience and type safety
    vk::CommandPoolCreateInfo commandPoolCreateInfo(
      vk::CommandPoolCreateFlagBits::eResetCommandBuffer, graphicsQueueFamily);

    init_with_deleter<vk::UniqueCommandPool>(context, commandPool, [&]() {
      return device.createCommandPoolUnique(commandPoolCreateInfo);
    });

    vk::CommandBufferAllocateInfo commandBufferAllocateInfo(
      commandPool.get(), vk::CommandBufferLevel::ePrimary, NUM_MAIN_CMD_BUFFERS);

    init_with_deleter_range<std::vector<vk::UniqueCommandBuffer>>(
      context, mainCommandBuffers, [&]() {
        return device.allocateCommandBuffersUnique(commandBufferAllocateInfo);
      });
  }

  void step7_create_default_renderpass([[maybe_unused]] VulkanBackend &context) {
    // This is the description of the image our renderpass will target
    vk::AttachmentDescription colorAttachment;

    // Use the same format as the swapchain
    colorAttachment.format = static_cast<vk::Format>(pSwapchain->image_format);

    // One sample; no MSAA
    colorAttachment.samples = vk::SampleCountFlagBits::e1;

    // Clear when the attachment is loaded
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;

    // Keep the attachment stored when the renderpass ends
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

    // Create the actual renderpass
    vk::RenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments    = &colorAttachment;
    renderPassCreateInfo.subpassCount    = 1;
    renderPassCreateInfo.pSubpasses      = &subpassDescription;

    init_with_deleter<vk::UniqueRenderPass>(
      context, renderPass, [&]() { return device.createRenderPassUnique(renderPassCreateInfo); });
  }

  void step8_create_framebuffers([[maybe_unused]] VulkanBackend &context) {
    const auto                windowDims = context.window_.dimensions();
    vk::FramebufferCreateInfo framebufferCreateInfo;
    framebufferCreateInfo.renderPass      = renderPass.get();
    framebufferCreateInfo.attachmentCount = 1;
    framebufferCreateInfo.width           = windowDims.first;
    framebufferCreateInfo.height          = windowDims.second;
    framebufferCreateInfo.layers          = 1;

    const auto swapchainImagesReturn = pSwapchain->get_images();
    validate_vkb_return(context, swapchainImagesReturn);
    const auto swapchainImages = swapchainImagesReturn.value();

    const auto swapchainImageViewsReturn = pSwapchain->get_image_views();
    validate_vkb_return(context, swapchainImageViewsReturn);
    swapchainImageViews = swapchainImageViewsReturn.value();

    // There seems to be no easy way to move the raw VkImageViews we get from vk-bootstrap into
    // vk::UniqueImageViews, so we have to be a little more hands on with destruction here. While we
    // can wrap the VkImageView in a vk::UniqueImageView, the destructor will trigger an assertion
    // because the vk::UniqueImageView will not be associated with our main vk::Device. We can call
    // vk::Device::CreateImageViewUnique, however that requires us to perform the full intialization
    // for the ImageView and would be redundant with the work that vk-bootstrap already did to get
    // us the swapchainImageViews.
    deleterFIFO.emplace_back([this]() {
      for(auto &swapchainImageView : swapchainImageViews) {
        vkDestroyImageView(pDevice->device, swapchainImageView, nullptr);
      }
    });

    init_with_deleter_range<std::vector<vk::UniqueFramebuffer>>(context, framebuffers, [&]() {
      std::vector<vk::UniqueFramebuffer> retval;
      for(auto &rawImageView : swapchainImageViews) {
        vk::ImageView imageView{rawImageView};
        framebufferCreateInfo.pAttachments = &imageView;
        retval.emplace_back(device.createFramebufferUnique(framebufferCreateInfo));
      }
      return retval;
    });
  }

  void step9_create_sync_structrues(VulkanBackend &context) {
    vk::FenceCreateInfo fenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);
    init_with_deleter<vk::UniqueFence>(
      context, renderFence, [&]() { return device.createFenceUnique(fenceCreateInfo); });

    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    init_with_deleter<vk::UniqueSemaphore>(
      context, renderSemaphore, [&] { return device.createSemaphoreUnique(semaphoreCreateInfo); });
    init_with_deleter<vk::UniqueSemaphore>(
      context, presentSemaphore, [&] { return device.createSemaphoreUnique(semaphoreCreateInfo); });
  }

  void validate_vk_return(VulkanBackend &context, VkResult result) {
    if(result != VK_SUCCESS) {
      std::stringstream ss;
      ss << "Vulkan failure; error code: " << vkErrorString(result);
      vk_fatal(context, ss.str());
    }
  }

  void validate_vk_return(VulkanBackend &context, vk::Result result) {
    validate_vk_return(context, VkResult(result));
  }

  template<typename T>
  // TODO: constrain me
  void validate_vkb_return(VulkanBackend &context, T &retval) {
    if(!retval) {
      std::stringstream ss;
      ss << "vk-bootstrap return failure for type "
         << util::TypeString<T> << "; reason: " << retval.error().message();
      vk_fatal(context, ss.str());
    }
  }

  void vk_fatal(VulkanBackend &context, const char *msg) {
    std::string errmsg = "Fatal Vulkan error: ";
    errmsg += msg;
    context.logger_.error(errmsg);
    throw std::runtime_error(errmsg);
  }

  void vk_fatal(VulkanBackend &context, const std::string &msg) { vk_fatal(context, msg.c_str()); }

  // Various stateful vulkan.hpp objects. Unique* objects will be cleaned up via the deleterFIFO,
  // whereas other references are Vulkan handles/other data which do not require cleanup
  std::vector<vk::UniqueCommandBuffer> mainCommandBuffers;
  vk::UniqueCommandPool                commandPool;
  vk::UniqueRenderPass                 renderPass;
  std::vector<VkImageView>             swapchainImageViews;
  std::vector<vk::UniqueFramebuffer>   framebuffers;
  vk::UniqueFence                      renderFence;
  vk::UniqueSemaphore                  renderSemaphore;
  vk::UniqueSemaphore                  presentSemaphore;
  vk::Queue                            graphicsQueue;
  U32                                  graphicsQueueFamily;
  VkSurfaceKHR                         surface;

  // N.B. that these are vulkan.hpp wrappers around non-owning references to vk-bootsrap objects,
  // which DO own the underlying Vulkan handles
  vk::Device       device;
  vk::SwapchainKHR swapchain;

  // vk-bootstrap objects
  std::unique_ptr<vkb::Device>     pDevice;
  std::unique_ptr<vkb::Instance>   pInstance;
  std::unique_ptr<vkb::Swapchain>  pSwapchain;
  std::unique_ptr<vkb::SystemInfo> pSystemInfo;

  /**
   * Use a FIFO to manage order of deletion for Vulkan objects; pattern borrowed from
   * https://vkguide.dev/docs/chapter-2/cleanup/
   *
   * This pattern enables us to declare member variables in any order, since we're managing order of
   * destruction ourselves and don't need to worry about in what order the compiler will place calls
   * to member variables' destructors.
   */
  std::vector<AgnosticDeleter_t> deleterFIFO;

  // N.B. we are currently creating only a single command buffer
  static constexpr auto NUM_MAIN_CMD_BUFFERS = 1;

  U64 numFrames;
};

VulkanBackend::VulkanBackend(ILogger &logger, PropertyMap &propertyMap, IWindow &window)
  : IGraphicsBackend(logger), propertyMap_(propertyMap), window_(window) {
  logger_.info("Initializing Vulkan, this may take a moment...");

  impl_->step1_query_system_info(*this);
  impl_->step2_create_instance(*this);
  impl_->step3_create_device(*this);
  impl_->step4_get_queues(*this);
  impl_->step5_create_swapchain(*this);
  impl_->step6_create_commands(*this);
  impl_->step7_create_default_renderpass(*this);
  impl_->step8_create_framebuffers(*this);
  impl_->step9_create_sync_structrues(*this);

  logger_.info("Vulkan initialization completed");
}

VulkanBackend::~VulkanBackend() { }

void VulkanBackend::render_frame() {
  impl_->validate_vk_return(
    *this, impl_->device.waitForFences(impl_->renderFence.get(), true, GPU_MAX_TIMEOUT_NS));
  impl_->device.resetFences(impl_->renderFence.get());

  // Don't call reset on the UniqueCommandBuffer; that's the std::unique_ptr API that will destroy
  // the object!
  impl_->mainCommandBuffers[0].get().reset();

  const auto swapchainImageIdx = impl_->device.acquireNextImageKHR(
    impl_->swapchain, GPU_MAX_TIMEOUT_NS, impl_->presentSemaphore.get());
  impl_->validate_vk_return(*this, swapchainImageIdx.result);

  auto                      &cmdBuffer = impl_->mainCommandBuffers[0].get();
  vk::CommandBufferBeginInfo cmdBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

  cmdBuffer.begin(cmdBufferBeginInfo);

  vk::RenderPassBeginInfo renderPassBeginInfo;
  renderPassBeginInfo.renderPass          = impl_->renderPass.get();
  renderPassBeginInfo.renderArea.offset.x = 0;
  renderPassBeginInfo.renderArea.offset.y = 0;

  const auto windowDim                         = window_.dimensions();
  renderPassBeginInfo.renderArea.extent.width  = windowDim.first;
  renderPassBeginInfo.renderArea.extent.height = windowDim.second;

  // TODO: validate swapchainImageIdx
  renderPassBeginInfo.framebuffer = impl_->framebuffers.at(swapchainImageIdx.value).get();

  vk::ClearValue clearValue;
  clearValue.color.setFloat32({
    {0.0f, 1.0f, 0.0f, 1.0f}
  });
  renderPassBeginInfo.clearValueCount = 1;
  renderPassBeginInfo.pClearValues    = &clearValue;

  cmdBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
  cmdBuffer.endRenderPass();

  // TODO: validate return
  cmdBuffer.end();

  vk::SubmitInfo               submitInfo;
  const vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  submitInfo.pWaitDstStageMask           = &waitStage;
  submitInfo.waitSemaphoreCount          = 1;
  submitInfo.pWaitSemaphores             = &(impl_->presentSemaphore.get());
  submitInfo.signalSemaphoreCount        = 1;
  submitInfo.pSignalSemaphores           = &(impl_->renderSemaphore.get());
  submitInfo.commandBufferCount          = 1;
  submitInfo.pCommandBuffers             = &cmdBuffer;

  impl_->graphicsQueue.submit(submitInfo, impl_->renderFence.get());

  vk::PresentInfoKHR presentInfo;
  presentInfo.pSwapchains        = &(impl_->swapchain);
  presentInfo.swapchainCount     = 1;
  presentInfo.pWaitSemaphores    = &(impl_->renderSemaphore.get());
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pImageIndices      = &(swapchainImageIdx.value);

  impl_->validate_vk_return(*this, impl_->graphicsQueue.presentKHR(presentInfo));
  {
    std::stringstream ss;
    ss << "Rendering frame #";
    ss << impl_->numFrames;
    logger_.trace(ss);
  }
  ++impl_->numFrames;
}

}  // namespace omulator