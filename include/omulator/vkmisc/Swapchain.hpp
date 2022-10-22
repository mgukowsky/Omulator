#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/IWindow.hpp"
#include "omulator/di/Injector.hpp"
#include "omulator/oml_types.hpp"
#include "omulator/util/Pimpl.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <vector>

namespace omulator::vkmisc {

/**
 * Holds Vulkan objects related to a swapchain, and has the ability to rebuild it as necessary.
 */
class Swapchain {
public:
  Swapchain(di::Injector &injector, ILogger &logger, IWindow &window, vk::raii::Device &device);
  ~Swapchain();

  vk::Format image_format();
  void       reset();

private:
  struct Impl_;
  util::Pimpl<Impl_> impl_;

  void build_framebuffers_();
  void build_image_views_();
  void build_swapchain_();

  void create_renderpass_();

  /**
   * Destroy internal structures in the reverse order of creation.
   */
  void clear_();

  di::Injector     &injector_;
  ILogger          &logger_;
  IWindow          &window_;
  vk::raii::Device &device_;

  std::vector<vk::raii::Framebuffer> framebuffers_;
  std::vector<VkImage>               images_;
  std::vector<vk::raii::ImageView>   imageViews_;
  vk::raii::RenderPass               renderPass_;
  vk::raii::SwapchainKHR             swapchain_;

  U32 surfaceWidth;
  U32 surfaceHeight;
};

}  // namespace omulator::vkmisc