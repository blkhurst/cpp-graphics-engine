#pragma once

#include <blkhurst/engine/config/defaults.hpp>
#include <blkhurst/engine/config/types.hpp>

namespace blkhurst {

struct WindowConfig {
  const char* title = defaults::window::title;
  GLVersion openGlVersion{defaults::opengl::version};
  glm::ivec2 size = defaults::window::size;
  glm::ivec2 pos = defaults::window::pos;
  int msaa = defaults::window::msaaSamples;
  bool enableVSync = defaults::window::vSync;
  glm::vec4 clearColor = defaults::window::clearColor;
};

} // namespace blkhurst
