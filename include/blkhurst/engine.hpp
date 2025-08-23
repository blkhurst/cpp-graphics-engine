#pragma once

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <iostream>

namespace blkhurst {

inline void printVersions() {
  std::cout << "GLFW version: " << glfwGetVersionString() << "\n";
  std::cout << "ImGui version: " << IMGUI_VERSION << "\n";
}

} // namespace blkhurst
