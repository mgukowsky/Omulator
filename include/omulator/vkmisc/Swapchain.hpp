#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/IWindow.hpp"
#include "omulator/di/Injector.hpp"
#include "omulator/oml_types.hpp"
#include "omulator/util/Pimpl.hpp"
#include "omulator/vkmisc/Allocator.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <vector>

namespace omulator::vkmisc {

/**
 * Holds Vulkan objects related to a swapchain, and has the ability to rebuild it as necessary.
 */
class Swapchain {
public:
  Swapchain(di::Injector     &injector,
            ILogger          &logger,
            IWindow          &window,
            Allocator        &allocator,
            vk::raii::Device &device);
  ~Swapchain();

  std::pair<U32, U32>     dimensions() const noexcept;
  vk::raii::Framebuffer  &framebuffer(const std::size_t idx);
  vk::Format              image_format();
  vk::raii::RenderPass   &renderPass();
  void                    reset();
  vk::raii::SwapchainKHR &swapchain();

private:
  struct Impl_;
  util::Pimpl<Impl_> impl_;

  void build_depth_image_views_();
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
  Allocator        &allocator_;
  vk::raii::Device &device_;

  AllocatedImage                     depthImage_;
  vk::raii::ImageView                depthImageView_;
  std::vector<vk::raii::Framebuffer> framebuffers_;
  std::vector<vk::raii::ImageView>   imageViews_;
  vk::raii::RenderPass               renderPass_;
  vk::raii::SwapchainKHR             swapchain_;

  vk::ImageCreateInfo     depthImageCreateInfo_;
  vk::ImageViewCreateInfo depthImageViewCreateInfo_;

  U32 surfaceWidth;
  U32 surfaceHeight;
};

}  // namespace omulator::vkmisc