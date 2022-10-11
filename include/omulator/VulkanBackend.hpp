#pragma once

#include "omulator/IGraphicsBackend.hpp"
#include "omulator/IWindow.hpp"
#include "omulator/PropertyMap.hpp"
#include "omulator/util/Pimpl.hpp"

namespace omulator {

/**
 * Backend for the Vulkan API.
 */
class VulkanBackend : public IGraphicsBackend {
public:
  VulkanBackend(ILogger &logger, PropertyMap &propertyMap, IWindow &window);
  ~VulkanBackend() override;

private:
  struct Impl_;
  util::Pimpl<Impl_> impl_;

  PropertyMap &propertyMap_;
  IWindow     &window_;
};
}  // namespace omulator