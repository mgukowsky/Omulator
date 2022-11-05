#include "omulator/vkmisc/SimpleMesh.hpp"

namespace {

constexpr auto ELEM_SIZE = sizeof(glm::vec3);

const std::vector ATTRS{
  vk::VertexInputAttributeDescription(0,
                                      0,
                                      vk::Format::eR32G32B32Sfloat,
                                      ELEM_SIZE *omulator::vkmisc::SimpleMesh::VERTEX_IDX_POSITION),
  vk::VertexInputAttributeDescription(
    1, 0, vk::Format::eR32G32B32Sfloat, ELEM_SIZE *omulator::vkmisc::SimpleMesh::VERTEX_IDX_NORMAL),
  vk::VertexInputAttributeDescription(
    2, 0, vk::Format::eR32G32B32Sfloat, ELEM_SIZE *omulator::vkmisc::SimpleMesh::VERTEX_IDX_COLOR)};

const std::vector BINDINGS{vk::VertexInputBindingDescription(
  0, sizeof(omulator::vkmisc::SimpleMesh::SimpleVertex), vk::VertexInputRate::eVertex)};

}  // namespace

namespace omulator::vkmisc {

SimpleMesh::SimpleMesh(ILogger &logger, Allocator &allocator, const std::size_t size)
  : buff_(logger,
          allocator,
          size,
          vk::BufferUsageFlagBits::eVertexBuffer,
          Allocator::Residency::CPU_TO_GPU) { }

vk::Buffer &SimpleMesh::buff() { return buff_.buff(); }

const std::vector<vk::VertexInputAttributeDescription> &SimpleMesh::get_attrs() const noexcept {
  return ATTRS;
}
const std::vector<vk::VertexInputBindingDescription> &SimpleMesh::get_bindings() const noexcept {
  return BINDINGS;
}

void SimpleMesh::set_vertex(const std::size_t idx, SimpleVertex vertex) {
  buff_.data().at(idx) = vertex;
}

std::size_t SimpleMesh::size() const noexcept { return buff_.size(); }

void SimpleMesh::upload() { buff_.upload(); }

}  // namespace omulator::vkmisc