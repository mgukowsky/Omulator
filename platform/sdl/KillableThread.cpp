#include "omulator/util/KillableThread.hpp"

#include <pthread.h>

#include <csignal>

namespace {
constexpr auto THRDKILLSIGNAL = SIGUSR1;
}  // namespace

namespace omulator::util {
KillableThread::KillableThread(std::function<void()> thrdProc)
  : thrd_{[thrdProc] {
      signal(THRDKILLSIGNAL, [](int) {
        int retval = 0;
        pthread_exit(&retval);
      });
      thrdProc();
    }} { }

KillableThread::~KillableThread() { pthread_kill(thrd_.native_handle(), THRDKILLSIGNAL); }
}  // namespace omulator::util