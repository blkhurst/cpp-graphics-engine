#include "window/window_theme.hpp"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define WIN32_LEAN_AND_MEAN
#include <GLFW/glfw3native.h>
#include <dwmapi.h>
#include <windows.h>
#endif

namespace blkhurst {

void enableWindowDarkMode(GLFWwindow* window) {
#ifdef _WIN32
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

  HWND hwnd = glfwGetWin32Window(window);
  BOOL value = true;
  DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
#endif
}

} // namespace blkhurst
