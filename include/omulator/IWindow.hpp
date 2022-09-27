#pragma once

class IWindow {
public:
  virtual ~IWindow()       = default;
  virtual void pump_msgs() = 0;
  virtual void show()      = 0;
};
