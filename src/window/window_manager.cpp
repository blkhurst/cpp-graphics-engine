#define GLFW_INCLUDE_NONE

#include "window/window_manager.hpp"
#include <blkhurst/engine/config/defaults.hpp>

#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <spdlog/spdlog.h>

#include <span>

namespace blkhurst {

WindowManager::WindowManager(const WindowConfig& config)
    : config_(config) {
  spdlog::debug("WindowManager initialising...");

  if (!WindowManager::initialiseGlfw()) {
    spdlog::critical("Failed to initialise GLFW");
    throw std::runtime_error("Failed to initialise GLFW.");
  }

  if (!WindowManager::initialiseGlad()) {
    spdlog::critical("Failed to initialise GLAD");
    throw std::runtime_error("Failed to initialise GLAD.");
  }

  configureOpenGL();
  spdlog::debug("WindowManager initialised");
}

WindowManager::~WindowManager() {
  if (window_ != nullptr) {
    spdlog::debug("Destroying GLFW window");
    glfwDestroyWindow(window_);
  }
  window_ = nullptr;

  spdlog::debug("Terminating GLFW");
  glfwTerminate();

  spdlog::debug("WindowManager shutdown");
}

// Public
GLFWwindow* WindowManager::getWindow() const {
  return window_;
}

GLVersion WindowManager::getOpenGlVersion() const {
  return config_.openGlVersion;
}

Resolution WindowManager::getFramebufferResolution() const {
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window_, &width, &height);
  spdlog::trace("Framebuffer size queried: {}x{}", width, height);
  return {width, height};
}

float WindowManager::getContentScale() const {
  GLFWmonitor* currentMonitor = getMonitorForWindow(window_);
  if (currentMonitor == nullptr) {
    spdlog::error("getContentScale failed: no monitor found for window");
    return 1.0;
  }

  float xscale = 1;
  float yscale = 1;
  glfwGetMonitorContentScale(currentMonitor, &xscale, &yscale);
  return xscale;
}

bool WindowManager::shouldClose() const {
  const bool closing = glfwWindowShouldClose(window_) != 0;
  if (closing) {
    spdlog::debug("Window received close request");
  }
  return closing;
}

void WindowManager::swapBuffersPollEvents() {
  glfwSwapBuffers(window_);
  glfwPollEvents();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// Private
bool WindowManager::initialiseGlfw() {
  if (glfwInit() == GLFW_FALSE) {
    spdlog::error("glfwInit() returned GLFW_FALSE");
    return false;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, config_.openGlVersion.major);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, config_.openGlVersion.minor);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, config_.msaa);
  glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

  glfwSetErrorCallback(WindowManager::errorCallback);

  window_ = glfwCreateWindow(config_.width, config_.height, config_.title, nullptr, nullptr);

  if (window_ == nullptr) {
    spdlog::error("glfwCreateWindow() returned null");
    glfwTerminate();
    return false;
  }

  spdlog::debug("GLFW window created ({}x{})", config_.width, config_.height);

  glfwMakeContextCurrent(window_);

  glfwSetFramebufferSizeCallback(window_, WindowManager::framebufferSizeCallback);

  glfwSwapInterval(static_cast<int>(config_.enableVSync));

  return true;
}

bool WindowManager::initialiseGlad() {
  int version = gladLoadGL(glfwGetProcAddress);
  if (version == 0) {
    spdlog::error("gladLoadGL() failed to load OpenGL");
    return false;
  }
  return true;
}

void WindowManager::configureOpenGL() const {
  const auto [r, g, b, a] = config_.clearColor;
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* Enable OpenGL Features */
  glEnable(GL_DEPTH_TEST);  // glDepthFunc(GL_LESS);
  glEnable(GL_MULTISAMPLE); // This does not apply anti-aliasing if using framebuffers.

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_STENCIL_TEST);

  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST); // Highest quality hint

  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

  spdlog::debug("OpenGL state configured (depth, MSAA, culling, blend, stencil, debug output)");
}

void WindowManager::useFullscreen(bool useFullscreen) {
  GLFWmonitor* currentMonitor = getMonitorForWindow(window_);
  if (currentMonitor == nullptr) {
    spdlog::error("useFullscreen({}) failed: no monitor found for window", useFullscreen);
    return;
  }

  if (useFullscreen) {
    const GLFWvidmode* mode = glfwGetVideoMode(currentMonitor);
    const int monitorWidth = mode->width;
    const int monitorHeight = mode->height;
    const int monitorRefresh = mode->refreshRate;

    spdlog::debug("Entering fullscreen on monitor '{}' ({}x{} @ {}Hz)",
                  glfwGetMonitorName(currentMonitor), monitorWidth, monitorHeight, monitorRefresh);

    glfwSetWindowMonitor(window_, currentMonitor, 0, 0, monitorWidth, monitorHeight,
                         monitorRefresh);
  } else {
    spdlog::debug("Exiting fullscreen to windowed mode ({}x{} at {},{})", config_.width,
                  config_.height, defaults::window::posX, defaults::window::posY);

    glfwSetWindowMonitor(window_, nullptr, defaults::window::posX, defaults::window::posY,
                         config_.width, config_.height, GLFW_DONT_CARE);
  }
}

GLFWmonitor* WindowManager::getMonitorForWindow(GLFWwindow* window) {
  int winX = 0;
  int winY = 0;
  glfwGetWindowPos(window, &winX, &winY);

  int count = 0;
  GLFWmonitor** monitors = glfwGetMonitors(&count);

  for (GLFWmonitor* mon : std::span(monitors, static_cast<size_t>(count))) {
    int monX = 0;
    int monY = 0;
    glfwGetMonitorPos(mon, &monX, &monY);

    const GLFWvidmode* mode = glfwGetVideoMode(mon);
    const int monWidth = mode->width;
    const int monHeight = mode->height;

    if (winX >= monX && winX < monX + monWidth && winY >= monY && winY < monY + monHeight) {
      spdlog::debug("Window is on monitor '{}'", glfwGetMonitorName(mon));
      return mon;
    }
  }
  spdlog::warn("Could not determine monitor for window");
  return nullptr;
}

void WindowManager::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
  if (width == 0 || height == 0) {
    spdlog::debug("Framebuffer size callback received zero dimension ({}x{}), skipping viewport",
                  width, height);
    return;
  }
  // glViewport lets OpenGL know how to map the NDC coordinates to the framebuffer coordinates
  glViewport(0, 0, width, height);
  spdlog::debug("Viewport updated to {}x{}", width, height);
}

void WindowManager::errorCallback(int error, const char* description) {
  spdlog::error("GLFW error [{}]: {}", error, (description != nullptr) ? description : "(null)");
}

} // namespace blkhurst
