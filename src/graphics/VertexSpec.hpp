#pragma once

#include "omulator/oml_types.hpp"

#include <vector>

namespace omulator::graphics {

// Struct describing the attributes for a single vertex
struct VertexAttributes {
  U32 binding;
  U32 format;
  U32 size;
};

// Struct describing the binding specifications given vertex types
// N.B. that the location of each vertex is DEDUCED based on its position within the
// vertexAttributes vector!
struct VertexSpec {
  std::vector<VertexAttributes> vertexAttributes;

  U32 stride;
  U32 inputRate;
};

}  // namespace omulator::graphics