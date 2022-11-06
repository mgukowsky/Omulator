#pragma once

#include "omulator/vkmisc/GPUBuffer.hpp"

#include <glm/glm.hpp>

#include <array>
#include <utility>
#include <vector>

namespace omulator::vkmisc {

class SimpleMesh {
public:
  /**
   * N.B. this is NOT an optimized vertex format!
   */
  using SimpleVertex    = std::array<glm::vec3, 3>;
  using PushConstants_t = std::pair<glm::vec4, glm::mat4>;

  static constexpr auto VERTEX_IDX_POSITION = 0;
  static constexpr auto VERTEX_IDX_NORMAL   = 1;
  static constexpr auto VERTEX_IDX_COLOR    = 2;

  SimpleMesh(ILogger &logger, Allocator &allocator, const std::size_t size);

  vk::Buffer &buff();

  const std::vector<vk::VertexInputAttributeDescription> &get_attrs() const noexcept;
  const std::vector<vk::VertexInputBindingDescription>   &get_bindings() const noexcept;

  std::size_t size() const noexcept;

  void set_vertex(const std::size_t idx, SimpleVertex vertex);

  void upload();

private:
  GPUBuffer<SimpleVertex> buff_;
};

}  // namespace omulator::vkmisc