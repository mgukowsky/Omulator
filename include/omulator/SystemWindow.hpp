#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/IWindow.hpp"
#include "omulator/InputHandler.hpp"
#include "omulator/util/Pimpl.hpp"

namespace omulator {
class SystemWindow : public IWindow {
public:
  SystemWindow(ILogger &logger, InputHandler &inputHandler);
  ~SystemWindow() override;

  void connect_to_graphics_api(IGraphicsBackend::GraphicsAPI graphicsApi,
                               void                         *pDataA,
                               void                         *pDataB) override;
  void pump_msgs() override;
  void show() override;

private:
  struct Impl_;
  util::Pimpl<Impl_> impl_;

  ILogger      &logger_;
  InputHandler &inputHandler_;

  static constexpr auto WINDOW_TITLE = "Omulator: the omnibus emulator";

  bool shown_;
};
}  // namespace omulator
