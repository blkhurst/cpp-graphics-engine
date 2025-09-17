#include "glfw_callbacks.hpp"
#include <blkhurst/input/input.hpp>

#include <imgui_impl_glfw.h>
#include <spdlog/spdlog.h>

namespace blkhurst {

static Input* get_input(GLFWwindow* window) {
  return static_cast<Input*>(glfwGetWindowUserPointer(window));
}

static void key_callback(GLFWwindow* win, int key, int sca, int act, int mod) {
  ImGui_ImplGlfw_KeyCallback(win, key, sca, act, mod);
  ImGuiIO& imguiIo = ImGui::GetIO();
  if (imguiIo.WantCaptureKeyboard) {
    return;
  }

  if (Input* input = get_input(win)) {
    if (act == GLFW_PRESS) {
      spdlog::trace("Key pressed ({})", key);
      input->pushKey(key, true);
    } else if (act == GLFW_RELEASE) {
      spdlog::trace("Key released ({})", key);
      input->pushKey(key, false);
    } else {
      // GLFW_REPEAT -> ignore for edge; level already true.
    }
  }
}

static void mouse_button_callback(GLFWwindow* win, int btn, int act, int mod) {
  ImGui_ImplGlfw_MouseButtonCallback(win, btn, act, mod);
  ImGuiIO& imguiIo = ImGui::GetIO();
  if (imguiIo.WantCaptureMouse) {
    return;
  }

  if (Input* input = get_input(win)) {
    if (act == GLFW_PRESS) {
      spdlog::trace("Mouse button pressed ({})", btn);
      input->pushMouseButton(btn, true);
    } else if (act == GLFW_RELEASE) {
      spdlog::trace("Mouse button released ({})", btn);
      input->pushMouseButton(btn, false);
    }
  }
}

static void cursor_pos_callback(GLFWwindow* win, double xpos, double ypos) {
  ImGui_ImplGlfw_CursorPosCallback(win, xpos, ypos);
  ImGuiIO& imguiIo = ImGui::GetIO();
  if (imguiIo.WantCaptureMouse) {
    return;
  }

  if (Input* input = get_input(win)) {
    input->pushMouseMove(xpos, ypos);
  }
}

static void scroll_callback(GLFWwindow* win, double xoffset, double yoffset) {
  ImGui_ImplGlfw_ScrollCallback(win, xoffset, yoffset);
  ImGuiIO& imguiIo = ImGui::GetIO();
  if (imguiIo.WantCaptureMouse) {
    return;
  }

  if (Input* input = get_input(win)) {
    input->pushScroll(xoffset, yoffset);
  }
}

static void char_callback(GLFWwindow* win, unsigned int codepoint) {
  ImGui_ImplGlfw_CharCallback(win, codepoint);
  ImGuiIO& imguiIo = ImGui::GetIO();
  if (imguiIo.WantTextInput) {
    return;
  }

  // ...
}

static void focus_callback(GLFWwindow* win, int focused) {
  if (Input* input = get_input(win)) {
    const bool hasFocus = (focused == GLFW_TRUE);
    input->setFocused(hasFocus);
  }
}

static void window_size_callback(GLFWwindow* win, int width, int height) {
  if (Input* input = get_input(win)) {
    input->pushWindowSize(width, height);
  }
}

static void framebuffer_size_callback(GLFWwindow* win, int width, int height) {
  if (Input* input = get_input(win)) {
    input->pushFramebufferSize(width, height);
  }
}

static void content_scale_callback(GLFWwindow* win, float xscale, float yscale) {
  if (Input* input = get_input(win)) {
    input->pushContentScale(xscale, yscale);
  }
}

void GlfwCallbacks::attach(GLFWwindow* win, Input& input) {
  if (win == nullptr) {
    spdlog::error("GlfwCallbacks::attach called with null window");
    return;
  }

  glfwSetWindowUserPointer(win, &input);

  // Keyboard
  glfwSetKeyCallback(win, key_callback);
  glfwSetCharCallback(win, char_callback);

  // Mouse
  glfwSetMouseButtonCallback(win, mouse_button_callback);
  glfwSetCursorPosCallback(win, cursor_pos_callback);
  glfwSetScrollCallback(win, scroll_callback);

  // Focus
  glfwSetWindowFocusCallback(win, focus_callback);

  // Window / DPI
  glfwSetWindowSizeCallback(win, window_size_callback);
  glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
  glfwSetWindowContentScaleCallback(win, content_scale_callback);
  // glfwSetWindowPosCallback

  input.setCursorModeSetter([win](blkhurst::CursorMode mode) { setCursorMode(win, mode); });

  spdlog::debug("GLFW callbacks attached");
}

void GlfwCallbacks::setCursorMode(GLFWwindow* win, CursorMode mode) {
  if (win == nullptr) {
    return;
  }

  switch (mode) {
  case CursorMode::Normal: {
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    spdlog::trace("CursorMode: Normal");
    break;
  }
  case CursorMode::Hidden: {
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    spdlog::trace("CursorMode: Hidden");
    break;
  }
  case CursorMode::Locked: {
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    spdlog::trace("CursorMode: Locked");
    break;
  }
  }
}

} // namespace blkhurst
