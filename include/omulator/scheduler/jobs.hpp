#pragma once

#include "omulator/oml_types.hpp"

#include <future>  // for std::packaged_task
#include <utility>

/**
 * This file contains definitions for miscellaneous types related to the scheduler.
 */

namespace omulator::scheduler {

/**
 * The priority of a task; where a higher value means greater priority.
 */
enum class Priority : U8 {
  IGNORE = 0,
  MIN    = 1,
  LOW    = 63,
  NORMAL = 127,
  HIGH   = 191,
  MAX    = 255,
};

/**
 * A structure representing a unit of work.
 */
struct Job_ty {
  static auto constexpr NULL_TASK = [] {};

  /**
   * A trivial constructor which performs no action and has minimal priority.
   */
  Job_ty() : task(NULL_TASK), priority(Priority::IGNORE) { }

  /**
   * We need a simple ctor here to handle templated callable args (helps us use emplace
   * functions elsewhere).
   */
  template<typename Callable>
  Job_ty(Callable &&callable, Priority priorityArg)
    : task(std::forward<Callable>(callable)), priority(priorityArg) { }

  std::packaged_task<void()> task;
  Priority                   priority;
};

} /* namespace omulator::scheduler */
