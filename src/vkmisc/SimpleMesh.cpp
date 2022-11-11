#include "omulator/vkmisc/SimpleMesh.hpp"

#include <tiny_obj_loader.h>

#include <vector>

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

SimpleMesh SimpleMesh::load_obj(ILogger &logger, Allocator &allocator, const std::string filepath) {
  tinyobj::attrib_t                attrib;
  std::vector<tinyobj::shape_t>    shapes;
  std::vector<tinyobj::material_t> materials;

  std::string warnStr;
  std::string errStr;

  tinyobj::LoadObj(&attrib, &shapes, &materials, &warnStr, &errStr, filepath.c_str(), nullptr);

  if(!warnStr.empty()) {
    logger.warn(warnStr);
  }
  if(!errStr.empty()) {
    logger.error(errStr);

    // TODO: don't throw; return some default mesh/print error
    throw std::runtime_error(errStr);
  }

  std::vector<SimpleVertex> vertices;

  for(auto &shape : shapes) {
    std::size_t indexOffset = 0;
    for(std::size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
      std::size_t fv = 3;

      for(std::size_t v = 0; v < fv; ++v) {
        tinyobj::index_t idx = shape.mesh.indices.at(indexOffset + v);

        tinyobj::real_t vx = attrib.vertices[static_cast<std::size_t>(3 * idx.vertex_index + 0)];
        tinyobj::real_t vy = attrib.vertices[static_cast<std::size_t>(3 * idx.vertex_index + 1)];
        tinyobj::real_t vz = attrib.vertices[static_cast<std::size_t>(3 * idx.vertex_index + 2)];
        tinyobj::real_t nx = attrib.normals[static_cast<std::size_t>(3 * idx.normal_index + 0)];
        tinyobj::real_t ny = attrib.normals[static_cast<std::size_t>(3 * idx.normal_index + 1)];
        tinyobj::real_t nz = attrib.normals[static_cast<std::size_t>(3 * idx.normal_index + 2)];

        vertices.emplace_back(SimpleVertex{
          glm::vec3{vx, vy, vz},
          glm::vec3{nx, ny, nz},
          glm::vec3{nx, ny, nz}
        });
      }

      indexOffset += fv;
    }
  }

  SimpleMesh newMesh(logger, allocator, vertices.size());
  for(std::size_t i = 0; i < vertices.size(); ++i) {
    newMesh.set_vertex(i, vertices.at(i));
  }

  return newMesh;
}

}  // namespace omulator::vkmisc
