#include "assets/loading_manager.hpp"
#include <blkhurst/renderer/renderer.hpp>

#include <spdlog/spdlog.h>

namespace blkhurst {

LoadingManager::LoadingManager(LoadingConfig config)
    : config_(config) {
  spdlog::debug("LoadingManager constructed with LoadMode({})", static_cast<int>(config_.loadMode));
}

LoadingManager::~LoadingManager() {
  spdlog::debug("LoadingManager destructed");
};

void LoadingManager::tick(const ClockInfo& tick, bool isLoading) {

  if (loadState_ == LoadState::FadingIn) {
    // If Asset Starts Loading Before Fade Completes, Jump To Loading
    if (isLoading) {
      setOpacity_(1.0F);
    }
    stepOpacity_(1.0F, tick.delta);
    if (opacity_ >= 1.0F) {
      loadState_ = LoadState::SceneSwitch;
      visibleSince_ = tick.elapsed;
    }
  }

  // if (loadState_ == LoadState::SceneSwitch) { }

  if (loadState_ == LoadState::Loading) {
    // Set visibleSince_
    if (opacity_ < 1.0F) {
      visibleSince_ = tick.elapsed;
    }
    // Visible Until Loading Completes & Min Time Elapsed
    setOpacity_(1.0F);
    const bool minTimeElapsed = (tick.elapsed - visibleSince_) >= config_.minDisplayTime;
    if (!isLoading && minTimeElapsed) {
      loadState_ = LoadState::FadingOut;
    }
  }

  if (loadState_ == LoadState::FadingOut) {
    // If Asset Starts Loading Before Fade Completes, Jump To Loading
    if (isLoading) {
      setOpacity_(1.0F);
    }
    //
    stepOpacity_(0.0F, tick.delta);
    if (opacity_ <= 0.0F) {
      loadState_ = LoadState::Completed;
    }
  }

  if (loadState_ == LoadState::Completed) {
    setOpacity_(0.0F);
    loadState_ = LoadState::Idle;
  }

  // Ensure LoadingScreen Opaque If isLoading
  const bool idleCompleted = (loadState_ == LoadState::Idle || loadState_ == LoadState::Completed);
  if (config_.showLoadingScreen && isLoading && idleCompleted) {
    if (opacity_ < 1.0F) {
      visibleSince_ = tick.elapsed;
    }
    setOpacity_(1.0F);
    loadState_ = LoadState::Loading;
  }
}

void LoadingManager::requestSceneChange(int sceneIndex, bool targetAlreadyLoaded) {
  targetAlreadyLoaded_ = targetAlreadyLoaded;
  pendingSceneChange_.store(sceneIndex, std::memory_order_relaxed);

  if (!config_.showLoadingScreen) {
    loadState_ = LoadState::SceneSwitch; // Skip Animation If Loading Screen Disabled
    return;
  }
  if (targetAlreadyLoaded_ && !config_.animateWhenLoaded) {
    loadState_ = LoadState::SceneSwitch; // Skip Animation If Target Already Loaded
    return;
  }

  loadState_ = LoadState::FadingIn;
}

[[nodiscard]] std::optional<int> LoadingManager::pendingSceneChange(int currentSceneIndex) {
  if (loadState_ != LoadState::SceneSwitch) {
    return std::nullopt;
  }

  const bool shouldAnimate =
      config_.showLoadingScreen && (!targetAlreadyLoaded_ || config_.animateWhenLoaded);
  loadState_ = shouldAnimate ? LoadState::Loading : LoadState::Completed;
  setOpacity_(shouldAnimate ? 1.0F : 0.0F);

  // Check If pendingSceneChange_ Is Valid & Return (CHECKED IN ENGINE)
  int sceneIndex = pendingSceneChange_.exchange(kNoActiveSceneIndex, std::memory_order_relaxed);
  if (sceneIndex == kNoActiveSceneIndex || sceneIndex == currentSceneIndex) {
    return std::nullopt;
  }
  return sceneIndex;
}

void LoadingManager::setLoadingScreen(std::unique_ptr<ILoadingScreen> loadingScreen) {
  loadingScreen_ = std::move(loadingScreen);
}

void LoadingManager::renderLoadingScreen(Renderer& renderer) {
  if (!shouldRender_) {
    return;
  }

  renderer.setAutoClear(false);
  renderer.render(*loadingScreen_, *loadingScreen_->camera());
  renderer.setAutoClear(true);
}

// --- Helpers

float LoadingManager::saturate(float val) {
  return val < 0.0F ? 0.0F : (val > 1.0F ? 1.0F : val);
}

void LoadingManager::setOpacity_(float alpha) {
  opacity_ = saturate(alpha);
  if (loadingScreen_) {
    loadingScreen_->setOpacity(opacity_);
  }
  shouldRender_ = (opacity_ > 0.0F);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void LoadingManager::stepOpacity_(float target, float delta) {
  const float dir = (target >= opacity_) ? 1.0F : -1.0F;
  const float duration = (dir > 0.0F) ? config_.fadeInDuration : config_.fadeOutDuration;
  if (duration <= 0.0F) {
    setOpacity_(target);
    return;
  }
  const float step = delta / duration;
  const float opacity = saturate(opacity_ + dir * step);
  setOpacity_(opacity);
}

} // namespace blkhurst
