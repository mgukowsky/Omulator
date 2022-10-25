#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/PropertyMap.hpp"
#include "omulator/vkmisc/Shader.hpp"
#include "omulator/vkmisc/Swapchain.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <memory>
#include <string>

namespace omulator::vkmisc {

class Pipeline {
public:
  enum class ShaderStage { VERTEX, FRAGMENT };

  Pipeline(ILogger          &logger,
           vk::raii::Device &device,
           Swapchain        &swapchain,
           PropertyMap      &propertyMap);

  vk::raii::Pipeline &pipeline();

  void rebuild_pipeline();

  vk::Rect2D &scissor();

  void set_shader(const ShaderStage shaderStage, const std::string shader);

  vk::Viewport &viewport();

private:

  void clear_();

  /**
   * Various constants for our pipeline state live here.
   */
  static constexpr std::array DYNAMIC_STATES_{vk::DynamicState::eViewport,
                                              vk::DynamicState::eScissor};

  ILogger          &logger_;
  vk::raii::Device &device_;
  Swapchain        &swapchain_;
  PropertyMap      &propertyMap_;

  vk::raii::Pipeline       pipeline_;
  vk::raii::PipelineLayout pipelineLayout_;

  /**
   * Initializer structs for the pipeline. These structs can be reused between calls to
   * rebuild_pipeline, while their state can be mutated in between.
   */
  vk::PipelineColorBlendAttachmentState    colorBlendAttachment_;
  vk::PipelineColorBlendStateCreateInfo    colorBlendInfo_;
  vk::PipelineDynamicStateCreateInfo       dynamicStateInfo_;
  vk::PipelineShaderStageCreateInfo        fragShaderInfo_;
  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo_;
  vk::PipelineMultisampleStateCreateInfo   multisamplingInfo_;
  vk::PipelineLayoutCreateInfo             pipelineLayoutInfo_;
  vk::PipelineRasterizationStateCreateInfo rasterizerInfo_;
  vk::Rect2D                               scissor_;
  vk::PipelineShaderStageCreateInfo        vertShaderInfo_;
  vk::PipelineVertexInputStateCreateInfo   vertexInputInfo_;
  vk::Viewport                             viewport_;
  vk::PipelineViewportStateCreateInfo      viewportInfo_;

  /**
   * Shaders
   */
  std::unique_ptr<Shader> fragmentShader_;
  std::unique_ptr<Shader> vertexShader_;
};

}  // namespace omulator::vkmisc