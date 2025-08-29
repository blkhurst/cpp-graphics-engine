#pragma once

#include <blkhurst/engine/config/defaults.hpp>
#include <blkhurst/engine/config/types.hpp>

namespace blkhurst {

struct WindowConfig {
  const char* title = defaults::windowTitle;
  GLVersion openGlVersion{defaults::openGlVersion};
  int width = defaults::windowWidth;
  int height = defaults::windowHeight;
  int msaa = defaults::msaaSamples;
  bool enableVSync = defaults::vSync;
  RGBA clearColor{defaults::clearColor};
};

} // namespace blkhurst
