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
                             vkmisc::DeviceQueues_t &deviceQueues,
                             vkmisc::Allocator      &allocator)
  : IGraphicsBackend(logger, injector),
    window_(window),
    device_(device),
    swapchain_(swapchain),
    pipeline_(pipeline),
    frameIdx_(0),
    needsResizing_(false),
    shouldRender_(false) {
  logger_.info("Initializing Vulkan, this may take a moment...");

  swapchain_.reset();

  for(std::size_t i = 0; i < vkmisc::NUM_FRAMES_IN_FLIGHT; ++i) {
    auto &cmdBuffs = injector.get<vk::raii::CommandBuffers>();
    assert(i < cmdBuffs.size()
           && "At least NUM_FRAMES_IN_FLIGHT command buffers need to be created");

    frames_.emplace_back(logger_,
                         i,
                         injector.creat_move<vk::raii::Fence>(),
                         injector.creat_move<vk::raii::Semaphore>(),
                         injector.creat_move<vk::raii::Semaphore>(),
                         device_,
                         cmdBuffs.at(i),
                         swapchain_,
                         pipeline_,
                         deviceQueues);
  }

  shouldRender_ = true;

  // TODO: make this a unique_ptr to shut the compiler up (or maybe make the allocation a ptr? seems
  // to be failing b/c of some vma magic...)
  meshes_.emplace_back(logger_, allocator, 3);
  auto &mesh = meshes_[0];
  pipeline_.set_vertex_binding_attrs(mesh.get_bindings(), mesh.get_attrs());
  set_vertex_shader("tri_mesh.vert.spv");
  mesh.set_vertex(
    0,
    {
      glm::vec3{1.0f, 1.0f, 0.0f},
      glm::vec3{0.0f, 0.0f, 0.0f},
      glm::vec3{0.0f, 1.0f, 0.0f}
  });
  mesh.set_vertex(
    1,
    {
      glm::vec3{-1.0f, 1.0f, 0.0f},
      glm::vec3{0.0f,  0.0f, 0.0f},
      glm::vec3{0.0f,  1.0f, 0.0f}
  });
  mesh.set_vertex(
    2,
    {
      glm::vec3{0.0f, -1.0f, 0.0f},
      glm::vec3{0.0f, 0.0f,  0.0f},
      glm::vec3{0.0f, 1.0f,  0.0f}
  });

  mesh.upload();

  logger_.info("Vulkan initialization completed");
}
VulkanBackend::~VulkanBackend() {
  // Wait for the GPU to finish before destroying all of the Vulkan components owned by this class.
  // N.B. that vk::raii plus our Injector will take care of destroying everything in the correct
  // order.
  wait_for_idle_();
}

IGraphicsBackend::GraphicsAPI VulkanBackend::api() const noexcept {
  return IGraphicsBackend::GraphicsAPI::VULKAN;
}

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

void VulkanBackend::render_frame() {
  vkmisc::Frame &currentFrame = frames_.at(frameIdx_);

  if(shouldRender_) {
    if(!(currentFrame.wait()) || needsResizing_) {
      do_resize_();
      needsResizing_ = false;
    }

    // On the off chance that rendering failed, we probably need to resize the window
    if(!currentFrame.render([&](vk::raii::CommandBuffer &cmdBuff) {
         auto          &mesh       = meshes_[0];
         vk::DeviceSize deviceSize = 0;
         cmdBuff.bindVertexBuffers(0, mesh.buff(), deviceSize);
         cmdBuff.draw(mesh.size(), 1, 0, 0);
       }))
    {
      needsResizing_ = true;
    }

    frameIdx_ = (frameIdx_ + 1) % vkmisc::NUM_FRAMES_IN_FLIGHT;
  }
}

void VulkanBackend::set_vertex_shader(const std::string &shader) {
  pipeline_.set_shader(vkmisc::Pipeline::ShaderStage::VERTEX, shader);
  wait_for_idle_();
  pipeline_.rebuild_pipeline();
}

void VulkanBackend::do_resize_() {
  // TODO: we need all frames to finish before it's safe to rebuild the swapchain, but would it be
  // better to call wait() for each frame in frames_ ?
  // OR... we could utilize the oldSwapChain field in VkSwapchainCreateInfoKHR while the old swap
  // chain images are still in flight on the GPU...
  wait_for_idle_();
  swapchain_.reset();
}

void VulkanBackend::upload_mesh_() { }

void VulkanBackend::wait_for_idle_() { device_.waitIdle(); }

}  // namespace omulator
