#pragma once

#include <blkhurst/engine/config/assets.hpp>
#include <blkhurst/engine/config/loading.hpp>
#include <blkhurst/engine/config/logger.hpp>
#include <blkhurst/engine/config/ui.hpp>
#include <blkhurst/engine/config/window.hpp>

namespace blkhurst {

struct EngineConfig {
  AssetsConfig assets{};
  LoggerConfig logger{};
  WindowConfig window{};
  UiConfig ui{};
  LoadingConfig loading{};
};

} // namespace blkhurst
