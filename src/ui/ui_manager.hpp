#pragma once

#include "window/window_manager.hpp"
#include <blkhurst/engine/config/ui.hpp>
#include <blkhurst/engine/root_state.hpp>
#include <blkhurst/events/event_bus.hpp>
#include <blkhurst/ui/ui_entry.hpp>

#include <imgui.h>
#include <string>

namespace blkhurst {

class UiManager {
public:
  UiManager(UiConfig config, const EventBus& events, const WindowManager& windowManager);
  ~UiManager();

  UiManager(const UiManager&) = delete;
  UiManager& operator=(const UiManager&) = delete;
  UiManager(UiManager&&) = delete;
  UiManager& operator=(UiManager&&) = delete;

  void beginFrame() const;
  void endFrame() const;

  void drawBaseUi(const RootState& state);
  void draw(UiEntry& entry, const RootState& state);

private:
  UiConfig config_;
  const EventBus& events_;
  const WindowManager& window_;

  float contentScale_ = 1.0F;

  void initialiseImGui(const WindowManager& windowManager);
  [[nodiscard]] std::string getGlVersionString() const;
  void setImGuiFont(const std::string& fontPath) const;

  void drawStatsHeader(const RootState& state);
  void drawScenesHeader(const RootState& state);

  static void defaultStyle();
};

} // namespace blkhurst
