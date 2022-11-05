#pragma once

#include "omulator/IGraphicsBackend.hpp"
#include "omulator/IWindow.hpp"
#include "omulator/PropertyMap.hpp"
#include "omulator/util/Pimpl.hpp"
#include "omulator/vkmisc/Allocator.hpp"
#include "omulator/vkmisc/Frame.hpp"
#include "omulator/vkmisc/Initializer.hpp"
#include "omulator/vkmisc/Pipeline.hpp"
#include "omulator/vkmisc/Shader.hpp"
#include "omulator/vkmisc/SimpleMesh.hpp"
#include "omulator/vkmisc/Swapchain.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <vector>

namespace omulator {

/**
 * Backend for the Vulkan API.
 */
class VulkanBackend : public IGraphicsBackend {
public:
  VulkanBackend(ILogger                &logger,
                di::Injector           &injector,
                IWindow                &window,
                vk::raii::Device       &device,
                vkmisc::Swapchain      &swapchain,
                vkmisc::Pipeline       &pipeline,
                vkmisc::DeviceQueues_t &deviceQueues,
                vkmisc::Allocator      &allocator);
  ~VulkanBackend() override;

  GraphicsAPI api() const noexcept override;

  void handle_resize() override;
  void render_frame() override;
  void set_vertex_shader(const std::string &shader) override;

private:
  IWindow           &window_;
  vk::raii::Device  &device_;
  vkmisc::Swapchain &swapchain_;
  vkmisc::Pipeline  &pipeline_;

  std::vector<vkmisc::Frame>      frames_;
  std::vector<vkmisc::SimpleMesh> meshes_;

  std::size_t frameIdx_;
  bool        needsResizing_;
  bool        shouldRender_;

  void do_resize_();

  void upload_mesh_();

  /**
   * Block until the GPU is finished executing its current workload. N.B. this is a rather coarse
   * synchronization mechanism and should be used sparingly.
   */
  void wait_for_idle_();
};
}  // namespace omulator
