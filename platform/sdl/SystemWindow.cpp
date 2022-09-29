#include "omulator/SystemWindow.hpp"

#include <SDL2/SDL.h>

#include <sstream>
#include <stdexcept>

namespace omulator {
struct SystemWindow::Impl_ {
  SDL_Window *pwnd;

  Impl_() : pwnd(nullptr) { }
  ~Impl_() = default;

  void sdl_fatal(ILogger &logger) {
    std::stringstream ss;
    ss << "Fatal SDL error: ";
    ss << SDL_GetError();
    logger.critical(ss);
    throw(std::runtime_error(ss.str()));
  }

  void check_sdl_return(ILogger &logger, int val) {
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
};

SystemWindow::SystemWindow(ILogger &logger, InputHandler &inputHandler)
  : logger_(logger), inputHandler_(inputHandler), shown_(false) {
  // Unlike Win32, SDL needs explicit initialization (and deinitalization in the destructor)
  impl_->check_sdl_return(logger_, SDL_Init(SDL_INIT_EVERYTHING));
}

SystemWindow::~SystemWindow() {
  if(impl_->pwnd != nullptr) {
    SDL_DestroyWindow(impl_->pwnd);
  }
  SDL_Quit();
}

void SystemWindow::pump_msgs() {
  SDL_Event event;
  while(SDL_PollEvent(&event)) {
    switch(event.type) {
      case SDL_KEYDOWN:
        impl_->handle_key(inputHandler_, event);
        break;
      default:
        break;
    }
  }
}

void SystemWindow::show() {
  if(shown_) {
    return;
  }

  impl_->pwnd = SDL_CreateWindow(
    WINDOW_TITLE,
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    640,
    480,
    SDL_WINDOW_VULKAN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  impl_->check_sdl_return(logger_, impl_->pwnd);

  shown_ = true;
}

}  // namespace omulator
