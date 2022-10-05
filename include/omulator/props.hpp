#pragma once

/**
 * String keys for various application properties.
 */
namespace omulator::props {

/**
 * If true, don't display a window.
 */
constexpr auto HEADLESS = "sys.headless";

/**
 * If true, accept commands on stdin.
 */
constexpr auto INTERACTIVE = "sys.interactive";

}  // namespace omulator::props