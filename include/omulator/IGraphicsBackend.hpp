#pragma once

#include "omulator/ILogger.hpp"

namespace omulator {

/**
 * Interface to a class responsible for interacting with a given graphics API.
 */
class IGraphicsBackend {
public:
  enum class GraphicsAPI { VULKAN };

  IGraphicsBackend(ILogger &logger) : logger_(logger) { }
  virtual ~IGraphicsBackend() = default;

  virtual void render_frame() = 0;

protected:
  ILogger &logger_;
};
}  // namespace omulator