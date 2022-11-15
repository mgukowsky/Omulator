#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/IWindow.hpp"
#include "omulator/PropertyMap.hpp"
#include "omulator/di/Injector.hpp"
#include "omulator/oml_types.hpp"
#include "omulator/util/Pimpl.hpp"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <map>
#include <utility>

namespace omulator::vkmisc {

/**
 * The maximum number of frames which can be submitted to the GPU at any given time. Also controls
 * the number of command buffers that are created. Set to 3 to play well with triple buffering.
 */
static constexpr std::size_t NUM_FRAMES_IN_FLIGHT = 3;

/**
 * Redeclare this enum from VkBootstrap so we don't have to expose that library in any of our
 * headers.
 */
enum class QueueType_t { present, graphics, compute, transfer };

/**
 * Maps a queue type to a pair of a Vulkan Queue and its queue index
 */
using DeviceQueues_t = std::map<QueueType_t, std::pair<vk::raii::Queue, const U32>>;

/**
 * Installs injector rules for vk::raii and omulator::vkmisc types
 */
void install_vk_initializer_rules(di::Injector &injector);
}  // namespace omulator::vkmisc
