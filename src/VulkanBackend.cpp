#include "omulator/VulkanBackend.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <cassert>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace { }  // namespace

namespace omulator {

// N.B. we make heavy use of vk::raii here, which is a level of abstraction above the core C API
// which provides a cleaner interface to Vulkan and handles API resource management.
struct VulkanBackend::Impl_ {
  Impl_() : pDevice(nullptr), pVkInstance(nullptr), pPhysicalDevice(nullptr) { }
  ~Impl_() { }

  void create_instance() {
    // Create the Vulkan context. N.B. that the VK C++ wrappers will just throw if any of these
    // fail, hence the absence of any error checking.
    vk::ApplicationInfo    applicationInfo("app", 1, "eng", 1, VK_API_VERSION_1_1);
    vk::InstanceCreateInfo instanceCreateInfo({}, &applicationInfo);
    pVkInstance = std::make_unique<vk::raii::Instance>(vctx, instanceCreateInfo);
  }

  void select_device(ILogger &logger) {
    vk::raii::PhysicalDevices physicalDevices(*pVkInstance);

    std::size_t deviceIdx = 0;
    std::size_t maxScore  = 0;

    if(physicalDevices.empty()) {
      throw std::runtime_error("Found 0 Vulkan physical devices");
    }
    else if(physicalDevices.size() > 1) {
      // Pick the best fit device; from
      // https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families
      std::size_t idx = 0;

      for(const auto &device : physicalDevices) {
        std::size_t score      = 0;
        const auto  properties = device.getProperties();
        if(properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
          score += 10000;
        }
        score += static_cast<std::size_t>(properties.limits.maxImageDimension2D);

        if(maxScore == 0 || score > maxScore) {
          deviceIdx = idx;
        }

        std::stringstream ss;
        ss << "Score for Vulkan physical device " << properties.deviceName << ": " << score;
        logger.info(ss);

        ++idx;
      }
    }

    assert(deviceIdx <= physicalDevices.size());
    pPhysicalDevice =
      std::make_unique<vk::raii::PhysicalDevice>(std::move(physicalDevices.at(deviceIdx)));

    std::stringstream ss;
    ss << physicalDevices.size() << " Vulkan physical devices found; using device " << deviceIdx
       << " (" << pPhysicalDevice->getProperties().deviceName << ")";
    logger.info(ss);
  }

  // Many vk::raii objects don't have default constructors, so we need to wrap them in unique_ptrs
  // to support delayed intialization.
  vk::raii::Context                         vctx;
  std::unique_ptr<vk::raii::Device>         pDevice;
  std::unique_ptr<vk::raii::Instance>       pVkInstance;
  std::unique_ptr<vk::raii::PhysicalDevice> pPhysicalDevice;
};

VulkanBackend::VulkanBackend(ILogger &logger) : IGraphicsBackend(logger) {
  impl_->create_instance();
  impl_->select_device(logger_);
}

VulkanBackend::~VulkanBackend() { }

}  // namespace omulator