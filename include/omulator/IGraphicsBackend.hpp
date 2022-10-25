#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/di/Injector.hpp"

namespace omulator {

/**
 * Interface to a class responsible for interacting with a given graphics API.
 */
class IGraphicsBackend {
public:
  enum class GraphicsAPI { VULKAN };

  IGraphicsBackend(ILogger &logger, di::Injector &injector)
    : logger_(logger), injector_(injector) { }
  virtual ~IGraphicsBackend() = default;

  virtual void handle_resize()                              = 0;
  virtual void render_frame()                               = 0;
  virtual void set_vertex_shader(const std::string &shader) = 0;

protected:
  ILogger      &logger_;
  di::Injector &injector_;
};
}  // namespace omulator