#pragma once

#include <vulkan/vulkan.hpp>

#include <filesystem>
#include <vector>

namespace omulator::vkmisc {

/**
 * Holds a single SPIR-V compiled shader loaded from the filesystem.
 */
class Shader {
public:
  Shader(vk::Device &device, std::filesystem::path path);

  const char *data() const noexcept;

  vk::ShaderModule &get() noexcept;

  std::size_t size() const noexcept;

private:
  vk::Device            &device_;
  std::vector<char>      buff_;
  vk::UniqueShaderModule shaderModule_;
};

}  // namespace omulator::vkmisc