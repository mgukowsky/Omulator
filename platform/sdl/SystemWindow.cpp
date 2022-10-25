#include "omulator/SystemWindow.hpp"

#include "omulator/oml_types.hpp"
#include "omulator/util/reinterpret.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan_raii.hpp>

#include <functional>
#include <sstream>
#include <stdexcept>
#include <utility>

using omulator::U32;
using WindowDim_t = std::pair<U32, U32>;

namespace {

WindowDim_t getWindowDimDefault(SDL_Window *pwnd) {
  int x, y;
  SDL_GetWindowSize(pwnd, &x, &y);

  return {static_cast<U32>(x), static_cast<U32>(y)};
}

WindowDim_t getWindowDimVk(SDL_Window *pwnd) {
  int x, y;
  SDL_Vulkan_GetDrawableSize(pwnd, &x, &y);

  return {static_cast<U32>(x), static_cast<U32>(y)};
}
}  // namespace

namespace omulator {
struct SystemWindow::Impl_ {
  SDL_Window *pwnd;

  // SDL has slightly different methods of determining window dimensions depending on the graphics
  // API in use
  std::function<WindowDim_t(SDL_Window *)> windowDimFn;

  Impl_() : pwnd(nullptr), windowDimFn(getWindowDimDefault) { }
  ~Impl_() = default;

  void sdl_fatal(ILogger &logger) {
    std::stringstream ss;
    ss << "Fatal SDL error: ";
    ss << SDL_GetError();
    logger.critical(ss);
    throw(std::runtime_error(ss.str()));
  }

  void check_sdl_return(ILogger &logger, const bool val) {
    if(!val) {
      sdl_fatal(logger);
    }
  }

  void check_sdl_return(ILogger &logger, const int val) {
    if(val < 0) {
      sdl_fatal(logger);
    }
  }

  template<typename T>
  void check_sdl_return(ILogger &logger, T *pT) {
    if(pT == nullptr) {
      sdl_fatal(logger);
    }
  }

  void handle_key(InputHandler &inputHandler, SDL_Event &event) {
    if(event.key.keysym.sym == SDLK_ESCAPE) {
      inputHandler.handle_input(InputHandler::InputEvent::QUIT);
    }
  }

  void handle_window_event(InputHandler &inputHandler, SDL_Event &event) {
    if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
      inputHandler.handle_input(InputHandler::InputEvent::RESIZE);
    }
  }
};

SystemWindow::SystemWindow(ILogger &logger, InputHandler &inputHandler)
  : logger_(logger), inputHandler_(inputHandler) {
  // Unlike Win32, SDL needs explicit initialization (and deinitalization in the destructor)
  impl_->check_sdl_return(logger_, SDL_Init(SDL_INIT_EVERYTHING));
}

SystemWindow::~SystemWindow() {
  if(impl_->pwnd != nullptr) {
    SDL_DestroyWindow(impl_->pwnd);
  }
  SDL_Quit();
}

void *SystemWindow::connect_to_graphics_api(IGraphicsBackend::GraphicsAPI graphicsApi,
                                            void                         *pData) {
  auto &instance = util::reinterpret<vk::raii::Instance>(pData);

  if(graphicsApi == IGraphicsBackend::GraphicsAPI::VULKAN) {
    VkSurfaceKHR surface;
    impl_->windowDimFn = getWindowDimVk;
    impl_->check_sdl_return(logger_, SDL_Vulkan_CreateSurface(impl_->pwnd, *instance, &surface));

    return surface;
  }
  else {
    impl_->windowDimFn = getWindowDimDefault;
    logger_.warn("Returning nullptr in SystemWindow::connect_to_graphics_api");
    return nullptr;
  }
}

std::pair<U32, U32> SystemWindow::dimensions() const noexcept {
  if(impl_->pwnd == nullptr) {
    logger_.warn("SystemWindow::dimensions called prior to window initialization");
    return {0, 0};
  }

  return impl_->windowDimFn(impl_->pwnd);
}

void *SystemWindow::native_handle() const noexcept { return impl_->pwnd; }

void SystemWindow::pump_msgs() {
  SDL_Event event;
  while(SDL_PollEvent(&event)) {
    switch(event.type) {
      case SDL_KEYDOWN:
        impl_->handle_key(inputHandler_, event);
        break;
      case SDL_QUIT:
        inputHandler_.handle_input(InputHandler::InputEvent::QUIT);
        break;
      case SDL_WINDOWEVENT:
        impl_->handle_window_event(inputHandler_, event);
        break;
      default:
        break;
    }
  }
}

void SystemWindow::show() {
  if(shown()) {
    return;
  }

  impl_->pwnd =
    SDL_CreateWindow(WINDOW_TITLE,
                     SDL_WINDOWPOS_UNDEFINED,
                     SDL_WINDOWPOS_UNDEFINED,
                     DEFAULT_WIDTH_,
                     DEFAULT_HEIGHT_,
                     SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  impl_->check_sdl_return(logger_, impl_->pwnd);

  set_shown_(true);
}

}  // namespace omulator
