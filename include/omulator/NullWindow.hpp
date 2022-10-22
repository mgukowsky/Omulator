#pragma once

#include "omulator/IWindow.hpp"
namespace omulator {

/**
 * Implements a headless window; useful for running Omulator in non-GUI environments.
 */
class NullWindow : public IWindow {
public:
  ~NullWindow() override = default;

  void *connect_to_graphics_api(IGraphicsBackend::GraphicsAPI graphicsApi, void *pData) override;

  std::pair<U32, U32> dimensions() const noexcept override;

  void pump_msgs() override;

  void show() override;
};

}  // namespace omulator