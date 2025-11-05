#pragma once
#include "engine/clock.hpp"
#include "scene/scene_manager.hpp"
#include <blkhurst/assets/loading_screen.hpp>
#include <blkhurst/engine/config/loading.hpp>

#include <atomic>
#include <optional>

namespace blkhurst {

class Renderer;

class LoadingManager {
public:
  LoadingManager(LoadingConfig config);
  ~LoadingManager();

  LoadingManager(const LoadingManager&) = delete;
  LoadingManager(LoadingManager&&) = delete;
  LoadingManager& operator=(const LoadingManager&) = delete;
  LoadingManager& operator=(LoadingManager&&) = delete;

  void tick(const ClockInfo& tick, bool isLoading);

  void requestSceneChange(int sceneIndex, bool targetAlreadyLoaded);
  [[nodiscard]] std::optional<int> pendingSceneChange(int currentSceneIndex);

  void setLoadingScreen(std::unique_ptr<ILoadingScreen> loadingScreen);
  void renderLoadingScreen(Renderer& renderer);

private:
  LoadingConfig config_{};
  std::atomic<int> pendingSceneChange_ = kNoActiveSceneIndex; // Called From Async Event

  // State
  enum class LoadState { Idle, FadingIn, SceneSwitch, Loading, FadingOut, Completed };
  LoadState loadState_ = LoadState::Idle;

  // LoadingScreen
  std::unique_ptr<ILoadingScreen> loadingScreen_ = DefaultLoadingScreen::create();
  float opacity_ = 0.0F;
  float visibleSince_ = 0.0F;
  bool targetAlreadyLoaded_ = false;
  bool shouldRender_ = false;

  // Helpers
  static float saturate(float val);
  void setOpacity_(float alpha);
  void stepOpacity_(float target, float delta);
};

} // namespace blkhurst
