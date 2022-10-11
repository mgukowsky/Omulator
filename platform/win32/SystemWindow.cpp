#include "omulator/SystemWindow.hpp"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <Windows.h>

#include <sstream>
#include <type_traits>

namespace omulator {
struct SystemWindow::Impl_ {
  HWND    hwnd;
  HMODULE hinstance;

  Impl_() : hwnd(nullptr), hinstance(GetModuleHandle(NULL)) { }
  ~Impl_() {
    if(hwnd != nullptr) {
      DestroyWindow(hwnd);
    }
  }

  static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    // Win32 magic to retrieve a pointer to the SystemWindow instance associated with the Win32
    // window.
    SystemWindow &context =
      *(reinterpret_cast<SystemWindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA)));

    LRESULT retval = 0;

    switch(msg) {
      case WM_CLOSE:
        context.inputHandler_.handle_input(InputHandler::InputEvent::QUIT);
        break;
      default:
        retval = DefWindowProc(hwnd, msg, wparam, lparam);
    }

    return retval;
  }

  void win32_fatal(ILogger &logger, std::string_view msg) {
    std::stringstream ss;
    ss << "Fatal Win32 error: ";
    ss << msg;
    logger.critical(ss);
    throw(std::runtime_error(ss.str()));
  }

  template<typename T>
  void check_win32_return(ILogger &logger, T val, std::string_view msg) {
    if constexpr(std::is_pointer_v<T>) {
      if(val == nullptr) {
        win32_fatal(logger, msg);
      }
    }
    else {
      if(val == NULL) {
        win32_fatal(logger, msg);
      }
    }
  }
};

SystemWindow::SystemWindow(ILogger &logger, InputHandler &inputHandler)
  : logger_(logger), inputHandler_(inputHandler), shown_(false) { }

SystemWindow::~SystemWindow() { }

void SystemWindow::connect_to_graphics_api(IGraphicsBackend::GraphicsAPI graphicsApi,
                                           void                         *pDataA,
                                           void                         *pDataB) {
  if(graphicsApi == IGraphicsBackend::GraphicsAPI::VULKAN) {
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd      = impl_->hwnd;
    createInfo.hinstance = impl_->hinstance;

    if(vkCreateWin32SurfaceKHR(*(reinterpret_cast<VkInstance *>(pDataA)),
                               &createInfo,
                               nullptr,
                               reinterpret_cast<VkSurfaceKHR *>(pDataB))
       != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create Win32 Vulkan surface");
    }
  }
}

void SystemWindow::pump_msgs() {
  MSG msg = {};
  while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

void SystemWindow::show() {
  if(shown_) {
    return;
  }

  WNDCLASSEX wcex = {
    .cbSize        = sizeof(WNDCLASSEX),
    .style         = CS_HREDRAW | CS_VREDRAW,
    .lpfnWndProc   = Impl_::wnd_proc,
    .cbClsExtra    = NULL,
    .cbWndExtra    = NULL,
    .hInstance     = impl_->hinstance,
    .hIcon         = LoadIcon(NULL, IDI_APPLICATION),
    .hCursor       = LoadCursor(NULL, IDC_ARROW),
    .hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH),
    .lpszMenuName  = NULL,
    .lpszClassName = WINDOW_TITLE,
    .hIconSm       = LoadIcon(NULL, IDI_APPLICATION),
  };

  impl_->check_win32_return(logger_, RegisterClassEx(&wcex), "RegisterClassEx");

  impl_->hwnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW,
                               WINDOW_TITLE,
                               WINDOW_TITLE,
                               WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               640,
                               480,
                               NULL,
                               NULL,
                               impl_->hinstance,
                               NULL);
  impl_->check_win32_return(logger_, impl_->hwnd, "CreateWindowEx");

  // Win32 magic to associate this Win32 window with this particular instance of SystemWindow.
  SetWindowLongPtr(impl_->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

  ShowWindow(impl_->hwnd, SW_SHOW);

  shown_ = true;
}

}  // namespace omulator
