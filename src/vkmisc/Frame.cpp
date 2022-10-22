#include "omulator/vkmisc/Frame.hpp"

namespace omulator::vkmisc {
Frame::Frame(const std::size_t        id,
             vk::raii::Fence          fence,
             vk::raii::Semaphore      presentSemaphore,
             vk::raii::Semaphore      renderSemaphore,
             vk::raii::CommandBuffer &cmdBuff)
  : id_(id),
    fence_(std::move(fence)),
    presentSemaphore_(std::move(presentSemaphore)),
    renderSemaphore_(std::move(renderSemaphore)),
    cmdBuff_(cmdBuff) { }
}  // namespace omulator::vkmisc