#include "mocks/PrimitiveIOMock.hpp"
#include "test/Sequencer.hpp"

#include <gtest/gtest.h>

#include <future>
#include <iostream>
#include <thread>

using omulator::test::Sequencer;

TEST(SmokeTest, sequencer_simple) {
  Sequencer sequencer(4);

  std::jthread t3([&] {
    sequencer.wait_for_step(3);
    EXPECT_EQ(3, sequencer.current_step())
      << "A Sequencer should implement synchronization points across multiple threads";
    sequencer.advance_step(4);
  });

  std::jthread t2([&] {
    sequencer.wait_for_step(2);
    EXPECT_EQ(2, sequencer.current_step())
      << "A Sequencer should implement synchronization points across multiple threads";
    sequencer.advance_step(3);
  });

  std::jthread t1([&] {
    sequencer.wait_for_step(1);
    EXPECT_EQ(1, sequencer.current_step())
      << "A Sequencer should implement synchronization points across multiple threads";
    sequencer.advance_step(2);
  });

  sequencer.advance_step(1);
  sequencer.wait_for_step(4);
  EXPECT_EQ(4, sequencer.current_step())
    << "A Sequencer should implement synchronization points across multiple threads";
}
