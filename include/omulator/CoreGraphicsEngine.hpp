#pragma once

#include "omulator/IGraphicsBackend.hpp"
#include "omulator/ILogger.hpp"
#include "omulator/Subsystem.hpp"
#include "omulator/di/Injector.hpp"
#include "omulator/msg/Message.hpp"

namespace omulator {

class CoreGraphicsEngine : public Subsystem {
public:
  explicit CoreGraphicsEngine(di::Injector &injector);
  ~CoreGraphicsEngine() = default;

private:
  di::Injector     &injector_;
  ILogger          &logger_;
  IGraphicsBackend &graphicsBackend_;
};

}  // namespace omulator