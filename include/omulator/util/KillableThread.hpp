#pragma once

#include <functional>
#include <thread>

namespace omulator::util {
/**
 * A class with a destructor which will terminate a thread in a platform-specific manner. Useful for
 * situations where a thread is blocked on an operation but needs to be stopped. Ideally this
 * function should still allow the std::jthread object to exit and be destroyed normally.
 */
class KillableThread {
public:
  explicit KillableThread(std::function<void()> thrdProc);
  ~KillableThread();

private:
  std::jthread thrd_;
};
}  // namespace omulator::util