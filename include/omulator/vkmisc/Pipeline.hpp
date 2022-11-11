#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/PropertyMap.hpp"
#include "omulator/vkmisc/Shader.hpp"
#include "omulator/vkmisc/Swapchain.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace omulator::vkmisc {

class Pipeline {
public:
  enum class ShaderStage { VERTEX, FRAGMENT };

  Pipeline(ILogger          &logger,
           vk::raii::Device &device,
           Swapchain        &swapchain,
           PropertyMap      &propertyMap);

  bool dirty() const noexcept;

  vk::raii::Pipeline       &pipeline();
  vk::raii::PipelineLayout &pipelineLayout();

  void rebuild_pipeline();

  vk::Rect2D &scissor();

  /**
   * Set a push constants.
   *
   * TODO: This will only set a single push constant at offset 0; add support for more! We also are
   * intentionally creating element 0 if it doesn't exist...
   */
  template<typename T>
  void set_push_constant(const vk::ShaderStageFlagBits shaderStage) {
    if(pushConstants_.empty()) {
      pushConstants_.emplace_back(vk::PushConstantRange());
    }
    auto &pushConstant      = pushConstants_.at(0);
    pushConstant.offset     = 0;
    pushConstant.size       = sizeof(T);
    pushConstant.stageFlags = shaderStage;

    pipelineLayoutInfo_.pushConstantRangeCount = static_cast<U32>(pushConstants_.size());
    pipelineLayoutInfo_.pPushConstantRanges    = pushConstants_.data();

    pipelineDirty_ = true;
  }

  /**
   * Set the shader used for a given stage in the pipeline. N.B. that rebuild_pipeline() will have
   * to be called in order for this change to take effect.
   */
  void set_shader(const ShaderStage shaderStage, const std::string shader);

  void set_vertex_binding_attrs(const std::vector<vk::VertexInputBindingDescription>   &bindings,
                                const std::vector<vk::VertexInputAttributeDescription> &attrs);
  /**
   * Updates dynamic state within the pipeline (i.e. state that may change per frame)
   */
  void update_dynamic_state();

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
   * Vertex info
   */
  std::vector<vk::VertexInputAttributeDescription> vertexAttributes_;
  std::vector<vk::VertexInputBindingDescription>   vertexBindings_;

  /**
   * Initializer structs for the pipeline. These structs can be reused between calls to
   * rebuild_pipeline, while their state can be mutated in between.
   */
  vk::PipelineColorBlendAttachmentState    colorBlendAttachment_;
  vk::PipelineColorBlendStateCreateInfo    colorBlendInfo_;
  vk::PipelineDepthStencilStateCreateInfo  depthStencilCreateInfo_;
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

  std::vector<vk::PushConstantRange> pushConstants_;

  /**
   * If true, then rebuild_pipeline_ should be invoked.
   */
  bool pipelineDirty_;
};

}  // namespace omulator::vkmisc
