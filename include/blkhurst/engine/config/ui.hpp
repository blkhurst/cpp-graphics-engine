#pragma once

#include <blkhurst/engine/config/defaults.hpp>
#include <string>

namespace blkhurst {

struct UiConfig {
  std::string title = defaults::ui::title;

  float fontSize = defaults::ui::fontSize;
  std::string fontPath = defaults::ui::fontPath;

  float scale = defaults::ui::scale;
  float minWindowWidth = defaults::ui::minWindowWidth;

  bool useDefaultStyle = defaults::ui::useDefaultStyle;
  bool showStatsHeader = defaults::ui::showStatsHeader;
  bool showScenesHeader = defaults::ui::showScenesHeader;
};

} // namespace blkhurst
