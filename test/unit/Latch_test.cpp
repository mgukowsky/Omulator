#include "omulator/Latch.hpp"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace {
// small amt of time to wait for a thread operation to advance.
constexpr auto THRD_DELAY = 10ms;
}  // namespace

TEST(Latch_test, basic_wait) {
  omulator::Latch l(1);
  EXPECT_FALSE(l.is_ready()) << "a Latch should not be ready before calling a wait function";

  std::atomic_bool done = false;

  std::promise<void> promise1, promise2;

  std::thread t([&] {
    promise1.set_value();
    l.wait();
    done = true;
    promise2.set_value();
  });

  promise1.get_future().wait();
  EXPECT_FALSE(done) << "Latch.wait() should block until the internal counter reaches 0";

  EXPECT_FALSE(l.is_ready()) << "a Latch should not be ready before the internal counter reaches 0";

  l.count_down();

  promise2.get_future().wait();
  EXPECT_TRUE(done)
    << "Functions waiting on a latch should unblock when the internal counter reaches 0";

  EXPECT_TRUE(l.is_ready()) << "a Latch should be ready when the internal counter reaches 0";

  t.join();
}

/**
 * The concurrency TS specifies that blocking should produce undefined behavior
 * after a Latch's destructor has been called. This example is a bit contrived, since
 * this behavior would indicate more serious dangling reference issues with the code in
 * question, none the less this test allows us to claim compliance with the concurrency
 * TS.
 */
TEST(Latch_test, wait_on_destroyed_latch) {
  omulator::Latch *pl;

  {
    omulator::Latch l(1);
    pl = &l;
  }

  EXPECT_THROW(pl->wait(), std::runtime_error)
    << "Latch::wait() cannot be called after Latch::~Latch()";
}

TEST(Latch_test, negative_latch_counter) {
  omulator::Latch l(1);
  EXPECT_THROW(l.count_down(2), std::runtime_error)
    << "a Latch's internal counter should not be allowed to decrease below 0";
}

TEST(Latch_test, advanced_count_down_and_wait) {
  constexpr std::size_t NUM_THREADS = 16;

  std::array<bool, NUM_THREADS> arrDone;
  omulator::Latch l(NUM_THREADS + 1);
  std::vector<std::thread> v;
  std::promise<void> promise1, promise2;

  for(std::size_t i = 0; i < NUM_THREADS; ++i) {
    arrDone[i] = false;

    v.emplace_back([&, i]() {
      l.count_down_and_wait();
      arrDone[i] = true;
    });
  }

  std::this_thread::sleep_for(THRD_DELAY);

  for(const auto &done : arrDone) {
    EXPECT_FALSE(done) << "Latch::count_down_and_wait() should block until the counter reaches 0";
  }

  l.count_down_and_wait();  // should return immediately
  EXPECT_TRUE(l.is_ready())
    << "Latch::count_down_and_wait() should return immediately when the counter becomes 0";

  std::this_thread::sleep_for(THRD_DELAY);

  for(const auto &done : arrDone) {
    EXPECT_TRUE(done) << "Latch::count_down_and_wait() should unblock when the counter reaches 0";
  }

  for(auto &t : v) {
    t.join();
  }
}
