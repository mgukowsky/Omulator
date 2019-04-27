#include "omulator/Window.hpp"

#include <unordered_map>

#include <Windows.h>

namespace {

  const char * const WND_CLASS_NAME = "OMLWND";
  constexpr DWORD WS_STYLES = WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME 
    | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
  constexpr DWORD WS_EX_STYLES = WS_EX_OVERLAPPEDWINDOW;

} /* namespace <anonymous> */

namespace omulator {

class Window::WindowImpl {
public:
  static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
      case WM_DESTROY:
        break;

      default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
  }
};

Window::Window() {
  WNDCLASSEX wcex;
  ZeroMemory(&wcex, sizeof(WNDCLASSEX));

  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.cbClsExtra = NULL;
  wcex.cbWndExtra = NULL;
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.hInstance = GetModuleHandle(NULL);
  wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.lpszClassName = WND_CLASS_NAME;
  wcex.lpszMenuName = nullptr;
  wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wcex.lpfnWndProc = WindowImpl::wnd_proc;
}

} /* namespace omulator */