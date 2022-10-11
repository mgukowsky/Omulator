#pragma once

#include "omulator/IGraphicsBackend.hpp"

namespace omulator {

/**
 * A generic interface for a GUI window.
 */
class IWindow {
public:
  virtual ~IWindow() = default;

  /**
   * Associate a window with a given graphics API.
   */
  virtual void connect_to_graphics_api(IGraphicsBackend::GraphicsAPI graphicsApi,
                                       void                         *pDataA,
                                       void                         *pDataB) = 0;

  /**
   * Process any messages sent to the Window by the OS or other sources.
   */
  virtual void pump_msgs() = 0;

  /**
   * Present the Window to the user, performing any additional initialization in the process. Should
   * return and perform no action if called more than once.
   */
  virtual void show() = 0;
};
}  // namespace omulator
