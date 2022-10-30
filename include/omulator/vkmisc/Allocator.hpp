#pragma once

#include "omulator/ILogger.hpp"
#include "omulator/util/Pimpl.hpp"

#include <vulkan/vulkan_raii.hpp>

namespace omulator::vkmisc {

class Allocator {
public:
  Allocator(ILogger                  &logger,
            vk::raii::Instance       &instance,
            vk::raii::PhysicalDevice &physicalDevice,
            vk::raii::Device         &device);
  ~Allocator();

private:
  struct Impl_;
  util::Pimpl<Impl_> impl_;

  ILogger &logger_;
};

}  // namespace omulator::vkmisc