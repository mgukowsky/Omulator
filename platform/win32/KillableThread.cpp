#include "omulator/util/KillableThread.hpp"

#include <Windows.h>

namespace omulator::util {

KillableThread::KillableThread(std::function<void()> thrdProc)
  : thrd_{[thrdProc] { thrdProc(); }} { }

KillableThread::~KillableThread() { TerminateThread(thrd_.native_handle(), 0); }
}  // namespace omulator::util