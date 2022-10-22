#pragma once

#include "omulator/IGraphicsBackend.hpp"
#include "omulator/IWindow.hpp"
#include "omulator/PropertyMap.hpp"
#include "omulator/util/Pimpl.hpp"
#include "omulator/vkmisc/Frame.hpp"
#include "omulator/vkmisc/Shader.hpp"
#include "omulator/vkmisc/Swapchain.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <vector>

namespace omulator {

/**
 * Backend for the Vulkan API.
 */
class VulkanBackend : public IGraphicsBackend {
public:
  VulkanBackend(ILogger           &logger,
                di::Injector      &injector,
                IWindow           &window,
                vk::raii::Device  &device,
                vkmisc::Swapchain &swapchain);
  ~VulkanBackend() override;

  void handle_resize() override;
  void render_frame() override;

private:
  IWindow           &window_;
  vk::raii::Device  &device_;
  vkmisc::Swapchain &swapchain_;

  std::vector<vkmisc::Frame> frames_;

  bool needsResizing_;
  bool shouldRender_;

  void do_resize_();
};
}  // namespace omulator