#pragma once

#include <string>

namespace blkhurst::events {

struct ToggleFullscreen {
  bool enabled = false;
};

struct SceneChange {
  std::string name;
  int index = -1;
};

} // namespace blkhurst::events
