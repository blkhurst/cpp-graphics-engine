#pragma once
#include <blkhurst/engine/config/defaults.hpp>

namespace blkhurst {

struct ScenesConfig {
  SceneLoadPolicy loadMode = defaults::scenes::loadMode;
  bool showLoadingScreen = defaults::scenes::showLoadingScreen;
};

} // namespace blkhurst
