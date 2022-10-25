#pragma once

#include "omulator/IGraphicsBackend.hpp"
#include "omulator/ILogger.hpp"
#include "omulator/IWindow.hpp"

namespace omulator::ui {

class IUIBackend {
public:
  IUIBackend(ILogger &logger, IWindow &window, IGraphicsBackend &graphicsBackend)
    : logger_(logger), window_(window), graphicsBackend_(graphicsBackend) { }
  virtual ~IUIBackend() = default;

protected:
  ILogger          &logger_;
  IWindow          &window_;
  IGraphicsBackend &graphicsBackend_;
};

}  // namespace omulator::ui