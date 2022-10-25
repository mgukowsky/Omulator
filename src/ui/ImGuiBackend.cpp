#include "omulator/ui/ImGuiBackend.hpp"

#include "omulator/util/reinterpret.hpp"

#ifdef _MSC_VER
#include <backends/imgui_impl_win32.h>
#else
#error Missing ImGui impl for platform
#endif

#include "omulator/vkmisc/Initializer.hpp"
#include "omulator/vkmisc/Swapchain.hpp"

#include <backends/imgui_impl_vulkan.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <imgui.h>

#include <array>

namespace {
constexpr auto DESCRIPTOR_COUNT = 1000;
}  // namespace

namespace omulator::ui {

ImGuiBackend::ImGuiBackend(ILogger          &logger,
                           IWindow          &window,
                           IGraphicsBackend &graphicsBackend,
                           di::Injector     &injector)
  : IUIBackend(logger, window, graphicsBackend), injector_(injector) {
  if(graphicsBackend_.api() == IGraphicsBackend::GraphicsAPI::VULKAN) {
    init_vulkan_();
  }
}

void ImGuiBackend::init_vulkan_() {
  std::array poolSizes{
    vk::DescriptorPoolSize{vk::DescriptorType::eSampler,              DESCRIPTOR_COUNT},
    vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, DESCRIPTOR_COUNT},
    vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage,         DESCRIPTOR_COUNT},
    vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage,         DESCRIPTOR_COUNT},
    vk::DescriptorPoolSize{vk::DescriptorType::eUniformTexelBuffer,   DESCRIPTOR_COUNT},
    vk::DescriptorPoolSize{vk::DescriptorType::eStorageTexelBuffer,   DESCRIPTOR_COUNT},
    vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer,        DESCRIPTOR_COUNT},
    vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer,        DESCRIPTOR_COUNT},
    vk::DescriptorPoolSize{vk::DescriptorType::eUniformBufferDynamic, DESCRIPTOR_COUNT},
    vk::DescriptorPoolSize{vk::DescriptorType::eStorageBufferDynamic, DESCRIPTOR_COUNT},
    vk::DescriptorPoolSize{vk::DescriptorType::eInputAttachment,      DESCRIPTOR_COUNT},
  };

  vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo(
    vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, DESCRIPTOR_COUNT, poolSizes);

  vk::raii::DescriptorPool imguiPool(injector_.get<vk::raii::Device>(), descriptorPoolCreateInfo);

  ImGui::CreateContext();
#ifdef _MSC_VER
  ImGui_ImplWin32_Init(window_.native_handle());
#else
#error Missing ImGui impl for platform
#endif

  ImGui_ImplVulkan_InitInfo initInfo;
  initInfo.Instance       = *(injector_.get<vk::raii::Instance>());
  initInfo.PhysicalDevice = *(injector_.get<vk::raii::PhysicalDevice>());
  initInfo.Device         = *(injector_.get<vk::raii::Device>());
  initInfo.Queue =
    *(injector_.get<vkmisc::DeviceQueues_t>().at(vkmisc::QueueType_t::graphics).first);
  initInfo.DescriptorPool = *imguiPool;
  initInfo.MinImageCount  = 3;
  initInfo.ImageCount     = 3;
  initInfo.MSAASamples    = VK_SAMPLE_COUNT_1_BIT;

  ImGui_ImplVulkan_Init(&initInfo, *(injector_.get<vkmisc::Swapchain>().renderPass()));

  // TODO: FINISH ME after implementing VulkanBackend::immediate_submit()
}

}  // namespace omulator::ui