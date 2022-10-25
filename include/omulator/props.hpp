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
 * Root directory for resources; defaults to the directory of the executable.
 */
constexpr auto RESOURCE_DIR = "sys.resource_dir";

/**
 * If true, turn on Vulkan debugging and validation.
 */
constexpr auto VKDEBUG = "sys.vkdebug";

/**
 * Working directory; defaults to the initial value of std::filesystem::current_path()
 */
constexpr auto WORKING_DIR = "sys.working_dir";

}  // namespace omulator::props