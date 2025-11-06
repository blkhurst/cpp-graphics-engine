#pragma once
#include "assets/loading_manager.hpp"
#include "engine/clock.hpp"
#include "scene/scene_manager.hpp"
#include "ui/ui_manager.hpp"
#include "window/window_manager.hpp"
#include <blkhurst/assets/asset_loader.hpp>
#include <blkhurst/engine.hpp>
#include <blkhurst/engine/config.hpp>
#include <blkhurst/engine/root_state.hpp>
#include <blkhurst/events/event_bus.hpp>
#include <blkhurst/input/input.hpp>
#include <blkhurst/renderer/renderer.hpp>

namespace blkhurst {

struct SceneContext {
  ClockInfo tick{};
  Scene* currentScene = nullptr;
  UUID currentSceneId = kInvalidUUID;
  Camera* currentCamera = nullptr;
  Controller* currentController = nullptr;
};

// PImpl
class Engine::Impl {
public:
  explicit Impl(const EngineConfig& config);
  void run();

  RootState buildRootState(const ClockInfo& tick, Scene* currentScene, Camera* currentCam);
  FrameUniforms buildFrameUniforms(const Input& input, const ClockInfo& tick, Camera* currentCam);
  SceneContext gatherSceneContext(Scene* scene);

  void drawUi(const RootState& rootState, Scene* currentScene);

  void setScene(const std::string& name);
  void registerSceneFactory(const std::string& name,
                            std::function<std::unique_ptr<Scene>()> factory);

private:
  // Initialisation order (ui_ must be after window_)
  EngineConfig config_;
  Clock clock_;
  EventBus events_;
  WindowManager window_;
  SceneManager scene_;
  UiManager ui_;
  Input input_;
  Renderer renderer_;
  AssetLoader assetLoader_;
  LoadingManager loadingManager_;

  std::vector<Subscription> subscriptions_;
  void registerEvents();

  // registerEvents helper
  template <class T, class Fn> void on(Fn&& callback) {
    subscriptions_.push_back(events_.subscribe<T>(std::forward<Fn>(callback)));
  }
};

} // namespace blkhurst
