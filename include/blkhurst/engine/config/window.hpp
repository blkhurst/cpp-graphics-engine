#pragma once

#include <blkhurst/engine/config/defaults.hpp>
#include <blkhurst/engine/config/types.hpp>

namespace blkhurst {

struct WindowConfig {
  const char* title = defaults::window::title;
  GLVersion openGlVersion{defaults::opengl::version};
  int width = defaults::window::width;
  int height = defaults::window::height;
  int msaa = defaults::window::msaaSamples;
  bool enableVSync = defaults::window::vSync;
  RGBA clearColor{defaults::window::clearColor};
};

} // namespace blkhurst
