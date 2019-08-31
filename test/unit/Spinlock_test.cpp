#include "omulator/util/Spinlock.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

TEST(Spinlock_test, basicLock) {
  omulator::util::Spinlock s;

  s.lock();
  EXPECT_FALSE(s.try_lock())
    << "Spinlock::lock() should acquire the lock";

  s.unlock();
  EXPECT_TRUE(s.try_lock())
    << "Spinlock::unlock() should release the lock";

  s.unlock();
}

TEST(Spinlock_test, tryLock) {
  omulator::util::Spinlock s;

  EXPECT_TRUE(s.try_lock())
    << "Spinlock::try_lock() should indicate whether or not the spinlock was acquired";

  EXPECT_FALSE(s.try_lock())
    << "Spinlock::try_lock() should reflect the state of the underlying primitive";

  s.unlock();

  EXPECT_TRUE(s.try_lock())
    << "Spinlock::try_lock() should reflect the state of the underlying primitive";

  s.unlock();
}

TEST(Spinlock_test, scopedLockCompliance) {
  omulator::util::Spinlock s;

  {
    std::scoped_lock sl(s);
    EXPECT_FALSE(s.try_lock())
      << "Spinlocks should behave as expected when used with std::scoped_lock";
  }

  EXPECT_TRUE(s.try_lock())
    << "Spinlocks should behave as expected when used with std::scoped_lock";

  s.unlock();
}
