#pragma once

#include "omulator/IGraphicsBackend.hpp"
#include "omulator/util/Pimpl.hpp"

namespace omulator {

/**
 * Backend for the Vulkan API.
 */
class VulkanBackend : public IGraphicsBackend {
public:
  VulkanBackend(ILogger &logger);
  ~VulkanBackend() override;

private:
  struct Impl_;
  util::Pimpl<Impl_> impl_;
};
}  // namespace omulator