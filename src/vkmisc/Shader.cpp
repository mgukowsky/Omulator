#include "omulator/vkmisc/Shader.hpp"

#include "omulator/oml_types.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace omulator::vkmisc {

Shader::Shader(ILogger                    &logger,
               vk::raii::Device           &device,
               std::string_view            filename,
               PropertyValue<std::string> &workingDir)
  : logger_(logger), device_(device), valid_(false), shaderModule_(nullptr) {
  // TODO: the path and shader dir should all be retrieved from PropertyMap!
  auto          filepath = std::filesystem::path(workingDir.get()) / SHADER_DIR / filename;
  std::ifstream ifs(filepath, std::ios::ate | std::ios::binary);

  if(!(ifs.is_open())) {
    std::string errmsg = "Failed to open file: ";
    errmsg += filepath.string();
    logger_.error(errmsg);
  }
  else {
    valid_          = true;
    std::string msg = "Successfully opened shader file: ";
    msg += filepath.string();
    logger_.info(msg);
  }

  if(valid_) {
    // Fancy method to effciently read a file into a buffer; from
    // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules#page_Loading-a-shader
    buff_.resize(static_cast<std::size_t>(ifs.tellg()));
    ifs.seekg(0);

    ifs.read(buff_.data(), static_cast<std::streamsize>(buff_.size()));

    vk::ShaderModuleCreateInfo shaderModuleCreateInfo(
      {}, size(), reinterpret_cast<const U32 *>(data()), nullptr);
    shaderModule_ = vk::raii::ShaderModule(device_, shaderModuleCreateInfo);
  }

  ifs.close();
}

const char *Shader::data() const noexcept { return buff_.data(); }

vk::raii::ShaderModule &Shader::get() noexcept { return shaderModule_; }

std::size_t Shader::size() const noexcept { return buff_.size(); }

bool Shader::valid() const noexcept { return valid_; }

}  // namespace omulator::vkmisc
