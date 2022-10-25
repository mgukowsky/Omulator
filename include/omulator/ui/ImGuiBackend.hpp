#pragma once

#include "omulator/di/Injector.hpp"
#include "omulator/ui/IUIBackend.hpp"

namespace omulator::ui {

class ImGuiBackend : public IUIBackend {
public:
  ImGuiBackend(ILogger          &logger,
               IWindow          &window,
               IGraphicsBackend &graphicsBackend,
               di::Injector     &injector);
  ~ImGuiBackend() override = default;

private:
  void          init_vulkan_();
  di::Injector &injector_;
};

}  // namespace omulator::ui