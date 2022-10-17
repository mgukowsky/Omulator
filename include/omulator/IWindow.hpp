#pragma once

#include "omulator/IGraphicsBackend.hpp"
#include "omulator/oml_types.hpp"

#include <atomic>
#include <utility>

namespace omulator {

/**
 * A generic interface for a GUI window.
 */
class IWindow {
public:
  IWindow() : shown_(false) { }
  virtual ~IWindow() = default;

  /**
   * Associate a window with a given graphics API.
   *
   * TODO: the two opaque handles we pass in to this function are perhaps a little too opaque, even
   * though they will mean different things depending on who calls this function and what graphics
   * API is selected... regardless, can we give the parameters more descriptive names?
   */
  virtual void connect_to_graphics_api(IGraphicsBackend::GraphicsAPI graphicsApi,
                                       void                         *pDataA,
                                       void                         *pDataB) = 0;

  /**
   * Get the dimensions of the window.
   */
  virtual std::pair<U32, U32> dimensions() const noexcept = 0;

  /**
   * Process any messages sent to the Window by the OS or other sources.
   */
  virtual void pump_msgs() = 0;

  /**
   * Present the Window to the user, performing any additional initialization in the process. Should
   * return and perform no action if called more than once.
   */
  virtual void show() = 0;

  /**
   * Returns true if the window is currently being displayed. Threadsafe.
   */
  bool shown() const noexcept { return shown_.load(std::memory_order_acquire); }

protected:
  void set_shown_(const bool newVal) noexcept { shown_.store(newVal, std::memory_order_release); }

private:
  std::atomic_bool shown_;
};
}  // namespace omulator
