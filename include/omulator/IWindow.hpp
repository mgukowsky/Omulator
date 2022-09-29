#pragma once

/**
 * A generic interface for a GUI window.
 */
class IWindow {
public:
  virtual ~IWindow() = default;

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
