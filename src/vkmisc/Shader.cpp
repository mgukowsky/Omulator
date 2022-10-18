#include "omulator/vkmisc/Shader.hpp"

#include "omulator/oml_types.hpp"

#include <fstream>
#include <stdexcept>

namespace omulator::vkmisc {

Shader::Shader(vk::Device &device, std::filesystem::path path) : device_(device) {
  std::ifstream ifs(path, std::ios::ate | std::ios::binary);

  if(!(ifs.is_open())) {
    std::string errmsg = "Failed to open file: ";
    errmsg += path.string();
    throw std::runtime_error(errmsg);
  }

  // Fancy method to effciently read a file into a buffer; from
  // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules#page_Loading-a-shader
  buff_.resize(ifs.tellg());
  ifs.seekg(0);

  ifs.read(buff_.data(), buff_.size());
  ifs.close();

  vk::ShaderModuleCreateInfo shaderModuleCreateInfo(
    {}, size(), reinterpret_cast<const U32 *>(data()), nullptr);
  shaderModule_ = device_.createShaderModuleUnique(shaderModuleCreateInfo);
}

const char *Shader::data() const noexcept { return buff_.data(); }

vk::ShaderModule &Shader::get() noexcept { return shaderModule_.get(); }

std::size_t Shader::size() const noexcept { return buff_.size(); }

}  // namespace omulator::vkmisc