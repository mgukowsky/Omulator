#include "omulator/NullWindow.hpp"

namespace omulator {
void NullWindow::connect_to_graphics_api([[maybe_unused]] IGraphicsBackend::GraphicsAPI graphicsApi,
                                         [[maybe_unused]] void                         *pDataA,
                                         [[maybe_unused]] void                         *pDataB) { }

void NullWindow::pump_msgs() { }

void NullWindow::show() { }
};  // namespace omulator