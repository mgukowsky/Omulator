#include "omulator/vkmisc/Pipeline.hpp"

#include "omulator/props.hpp"

#include <array>

namespace omulator::vkmisc {

Pipeline::Pipeline(ILogger          &logger,
                   vk::raii::Device &device,
                   Swapchain        &swapchain,
                   PropertyMap      &propertyMap)
  : logger_(logger),
    device_(device),
    swapchain_(swapchain),
    propertyMap_(propertyMap),
    pipeline_(nullptr),
    pipelineLayout_(nullptr) {
  // We have a relatively chunky constructor here, whose primary responsibility is to fill out
  // various initializer structs with information which will not change between calls to
  // rebuild_pipeline().

  // Dynamic states do not need to be baked into the pipeline, but need to be specified at draw
  // time.
  dynamicStateInfo_.dynamicStateCount = static_cast<U32>(DYNAMIC_STATES_.size());
  dynamicStateInfo_.pDynamicStates    = DYNAMIC_STATES_.data();

  // Initialize with no vertex input data
  vertexInputInfo_.vertexBindingDescriptionCount   = 0;
  vertexInputInfo_.pVertexBindingDescriptions      = nullptr;
  vertexInputInfo_.vertexAttributeDescriptionCount = 0;
  vertexInputInfo_.pVertexAttributeDescriptions    = nullptr;

  inputAssemblyInfo_.topology               = vk::PrimitiveTopology::eTriangleList;
  inputAssemblyInfo_.primitiveRestartEnable = false;

  // N.B. that, because we are using dynamic state for the viewport and scissor, we don't need a
  // vk::ViewportStateCreateInfo struct!
  {
    // Grab the dims from the swapchain rather than the window to avoid a very unlikely but possible
    // scenario where the swapchain would end up with different dims than what the window would have
    // at this moment.
    const auto dims = swapchain_.dimensions();

    // Fairly standard viewport
    viewport_.x        = 0;
    viewport_.y        = 0;
    viewport_.width    = static_cast<float>(dims.first);
    viewport_.height   = static_cast<float>(dims.second);
    viewport_.minDepth = 0.0f;
    viewport_.maxDepth = 1.0f;

    // Nothing special with the scissor
    scissor_.offset = vk::Offset2D{0, 0};
    scissor_.extent = vk::Extent2D{dims.first, dims.second};
  }

  viewportInfo_.viewportCount = 1;
  viewportInfo_.scissorCount  = 1;

  rasterizerInfo_.depthClampEnable = false;

  // N.B. if this is true, then no geometry will ever be passed through the rasterizer
  rasterizerInfo_.rasterizerDiscardEnable = false;

  // N.B. that any mode other than fill requires enabling a feature
  rasterizerInfo_.polygonMode = vk::PolygonMode::eFill;

  // N.B. that lines thicker than 1.0f requires enabling a feature
  rasterizerInfo_.lineWidth = 1.0f;

  rasterizerInfo_.cullMode                = vk::CullModeFlagBits::eBack;
  rasterizerInfo_.frontFace               = vk::FrontFace::eClockwise;
  rasterizerInfo_.depthBiasEnable         = false;
  rasterizerInfo_.depthBiasConstantFactor = 0.0f;
  rasterizerInfo_.depthBiasClamp          = 0.0f;
  rasterizerInfo_.depthBiasSlopeFactor    = 0.0f;

  // Start w/o multisampling; N.B. that utilizing it requires a GPU feature
  multisamplingInfo_.sampleShadingEnable   = false;
  multisamplingInfo_.rasterizationSamples  = vk::SampleCountFlagBits::e1;
  multisamplingInfo_.minSampleShading      = 1.0f;
  multisamplingInfo_.pSampleMask           = nullptr;
  multisamplingInfo_.alphaToCoverageEnable = false;
  multisamplingInfo_.alphaToOneEnable      = false;

  colorBlendAttachment_.colorWriteMask =
    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
    | vk::ColorComponentFlagBits::eA;
  colorBlendAttachment_.blendEnable         = false;
  colorBlendAttachment_.srcColorBlendFactor = vk::BlendFactor::eOne;
  colorBlendAttachment_.dstColorBlendFactor = vk::BlendFactor::eZero;
  colorBlendAttachment_.colorBlendOp        = vk::BlendOp::eAdd;
  colorBlendAttachment_.srcAlphaBlendFactor = vk::BlendFactor::eOne;
  colorBlendAttachment_.dstAlphaBlendFactor = vk::BlendFactor::eZero;
  colorBlendAttachment_.alphaBlendOp        = vk::BlendOp::eAdd;

  colorBlendInfo_.logicOpEnable   = false;
  colorBlendInfo_.logicOp         = vk::LogicOp::eCopy;
  colorBlendInfo_.attachmentCount = 1;
  colorBlendInfo_.pAttachments    = &colorBlendAttachment_;
  colorBlendInfo_.blendConstants  = std::array{0.0f, 0.0f, 0.0f, 0.0f};

  pipelineLayoutInfo_.setLayoutCount         = 0;
  pipelineLayoutInfo_.pSetLayouts            = nullptr;
  pipelineLayoutInfo_.pushConstantRangeCount = 0;
  pipelineLayoutInfo_.pPushConstantRanges    = nullptr;

  vertShaderInfo_.stage = vk::ShaderStageFlagBits::eVertex;
  vertShaderInfo_.pName = Shader::ENTRY_POINT;

  fragShaderInfo_.stage = vk::ShaderStageFlagBits::eFragment;
  fragShaderInfo_.pName = Shader::ENTRY_POINT;

  // Load default shaders
  // TODO: put more sensible defaults here
  set_shader(ShaderStage::VERTEX, "triangle.vert.spv");
  set_shader(ShaderStage::FRAGMENT, "triangle.frag.spv");

  rebuild_pipeline();
}

vk::raii::Pipeline &Pipeline::pipeline() { return pipeline_; }

// void Pipeline::add_shader(const Pipeline::ShaderStage shaderStage, const std::string shader) { }

void Pipeline::rebuild_pipeline() {
  clear_();
  pipelineLayout_ = vk::raii::PipelineLayout(device_, pipelineLayoutInfo_);

  std::array shaderStages{vertShaderInfo_, fragShaderInfo_};

  vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
  pipelineCreateInfo.stageCount          = static_cast<U32>(shaderStages.size());
  pipelineCreateInfo.pStages             = shaderStages.data();
  pipelineCreateInfo.pVertexInputState   = &vertexInputInfo_;
  pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo_;
  pipelineCreateInfo.pViewportState      = &viewportInfo_;
  pipelineCreateInfo.pRasterizationState = &rasterizerInfo_;
  pipelineCreateInfo.pMultisampleState   = &multisamplingInfo_;
  pipelineCreateInfo.pDepthStencilState  = nullptr;
  pipelineCreateInfo.pColorBlendState    = &colorBlendInfo_;
  pipelineCreateInfo.pDynamicState       = &dynamicStateInfo_;
  pipelineCreateInfo.layout              = *pipelineLayout_;

  // TODO: handle multiple subpasses more elegantly
  pipelineCreateInfo.renderPass = *(swapchain_.renderPass());
  pipelineCreateInfo.subpass    = 0;

  // TODO: use these to make it easier to recreate the pipeline on-demand
  pipelineCreateInfo.basePipelineHandle = nullptr;
  pipelineCreateInfo.basePipelineIndex  = -1;

  pipeline_ = vk::raii::Pipeline(device_, nullptr, pipelineCreateInfo);
}

vk::Rect2D &Pipeline::scissor() { return scissor_; }

void Pipeline::set_shader(const Pipeline::ShaderStage shaderStage, const std::string shader) {
  std::unique_ptr newShader = std::make_unique<Shader>(
    logger_, device_, shader, propertyMap_.get_prop<std::string>(props::RESOURCE_DIR));

  if(shaderStage == ShaderStage::FRAGMENT) {
    fragmentShader_.reset(nullptr);
    fragmentShader_        = std::move(newShader);
    fragShaderInfo_.module = *(fragmentShader_->get());
  }
  else {
    vertexShader_.reset(nullptr);
    vertexShader_          = std::move(newShader);
    vertShaderInfo_.module = *(vertexShader_->get());
  }
}

vk::Viewport &Pipeline::viewport() { return viewport_; }

void Pipeline::clear_() {
  pipeline_.clear();
  pipelineLayout_.clear();
}

}  // namespace omulator::vkmisc