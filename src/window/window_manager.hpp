#pragma once

#include "window/types.hpp"
#include <blkhurst/engine/config.hpp>

struct GLFWwindow;
struct GLFWmonitor;

namespace blkhurst {

class WindowManager {
public:
  WindowManager(const WindowConfig& config);
  ~WindowManager();

  WindowManager(const WindowManager&) = delete;
  WindowManager& operator=(const WindowManager&) = delete;
  WindowManager(WindowManager&&) = delete;
  WindowManager& operator=(WindowManager&&) = delete;

  [[nodiscard]] GLFWwindow* getWindow() const;
  [[nodiscard]] GLVersion getOpenGlVersion() const;
  [[nodiscard]] Resolution getFramebufferResolution() const;
  [[nodiscard]] float getContentScale() const;
  [[nodiscard]] bool shouldClose() const;

  void useFullscreen(bool useFullscreen);
  void swapBuffersPollEvents();

private:
  GLFWwindow* window_ = nullptr;
  WindowConfig config_;

  bool initialiseGlfw();
  static bool initialiseGlad();
  void configureOpenGL() const;

  static GLFWmonitor* getMonitorForWindow(GLFWwindow* window);

  static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
  static void errorCallback(int error, const char* description);
};

} // namespace blkhurst
