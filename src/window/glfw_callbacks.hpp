#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <blkhurst/input/input.hpp>

namespace blkhurst {

struct GlfwCallbacks {
  static void attach(GLFWwindow* win, Input& inp);
  static void setCursorMode(GLFWwindow* win, CursorMode mode);
};

} // namespace blkhurst
