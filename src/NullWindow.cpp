#include "omulator/NullWindow.hpp"

namespace omulator {
void NullWindow::connect_to_graphics_api([[maybe_unused]] IGraphicsBackend::GraphicsAPI graphicsApi,
                                         [[maybe_unused]] void                         *pDataA,
                                         [[maybe_unused]] void                         *pDataB) { }

std::pair<U32, U32> NullWindow::dimensions() const noexcept { return {0, 0}; }

void NullWindow::pump_msgs() { }

void NullWindow::show() { set_shown_(true); }
};  // namespace omulator