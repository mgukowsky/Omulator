#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/PropertyMap.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace omulator::vkmisc {

/**
 * Holds a single SPIR-V compiled shader loaded from the filesystem.
 */
class Shader {
public:
  static constexpr auto ENTRY_POINT = "main";
  static constexpr auto SHADER_DIR  = "shaders";

  Shader(ILogger                    &logger,
         vk::raii::Device           &device,
         std::string_view            filename,
         PropertyValue<std::string> &workingDir);

  const char *data() const noexcept;

  vk::raii::ShaderModule &get() noexcept;

  std::size_t size() const noexcept;

private:
  ILogger               &logger_;
  vk::raii::Device      &device_;
  std::vector<char>      buff_;
  vk::raii::ShaderModule shaderModule_;
};

}  // namespace omulator::vkmisc