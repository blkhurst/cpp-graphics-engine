#pragma once
#include <blkhurst/engine/config/defaults.hpp>

namespace blkhurst {

struct LoadingConfig {
  SceneLoadPolicy loadMode = defaults::loading::loadMode;
  bool showLoadingScreen = defaults::loading::showLoadingScreen;
  bool animateWhenLoaded = defaults::loading::animateWhenLoaded;
  float fadeInDuration = defaults::loading::fadeInDuration;
  float fadeOutDuration = defaults::loading::fadeOutDuration;
  float minDisplayTime = defaults::loading::minDisplayTime;
};

} // namespace blkhurst
