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

/**
 * If true, turn on Vulkan debugging and validation.
 */
constexpr auto VKDEBUG = "sys.vkdebug";

}  // namespace omulator::props