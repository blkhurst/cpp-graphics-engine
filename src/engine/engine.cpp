#include "engine/clock.hpp"
#include "logging/logger.hpp"
#include "scene/scene_manager.hpp"
#include "ui/ui_manager.hpp"
#include "window/window_manager.hpp"
#include <blkhurst/engine.hpp>
#include <blkhurst/engine/config.hpp>
#include <blkhurst/engine/root_state.hpp>
#include <blkhurst/events/event_bus.hpp>
#include <blkhurst/events/events.hpp>
#include <blkhurst/util/assets.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>

namespace blkhurst {

// PImpl
class Engine::Impl {
public:
  explicit Impl(const EngineConfig& cfg)
      : config_(cfg),
        window_(cfg.windowConfig),
        ui_(cfg.uiConfig, events_, window_) {
    registerEvents();
  }

  void run() {
    while (!window_.shouldClose()) {
      const auto tick = clock_.tick();

      auto* currentScene = scene_.currentScene();

      RootState rootState = {
          .delta = tick.delta,
          .elapsed = tick.elapsed,
          .fps = tick.fps,
          .ms = tick.ms,
          .renderer = nullptr,
          .camera = nullptr,
          .input = nullptr,
          .scene = currentScene,
          .events = &events_,
          .currentSceneIndex = scene_.currentIndex(),
          .sceneNames = scene_.names(),
      };

      if (currentScene != nullptr) {
        currentScene->traverse([&](Object3D& node) { node.onUpdate(rootState); });
      }

      ui_.beginFrame();
      ui_.drawBaseUi(rootState);
      if (currentScene != nullptr) {
        for (const auto& uiEntry : currentScene->uiEntries()) {
          ui_.draw(*uiEntry, rootState);
        }
      }
      ui_.endFrame();

      window_.swapBuffersPollEvents();
    }
  }

  void registerSceneFactory(const std::string& name,
                            std::function<std::unique_ptr<Scene>()> factory) {
    scene_.registerFactory(name, std::move(factory));
  }

private:
  // Initialisation order (ui_ must be after window_)
  EngineConfig config_;
  Clock clock_;
  EventBus events_;
  WindowManager window_;
  SceneManager scene_;
  UiManager ui_;

  std::vector<Subscription> subscriptions_;

  void registerEvents() {
    using namespace events;
    on<SceneChange>([this](const SceneChange& scene) { scene_.setScene(scene.index); });
    on<ToggleFullscreen>(
        [this](const ToggleFullscreen& fullscreen) { window_.useFullscreen(fullscreen.enabled); });
  }

  // registerEvents helper
  template <class T, class Fn> void on(Fn&& callback) {
    subscriptions_.push_back(events_.subscribe<T>(std::forward<Fn>(callback)));
  }
};

// Public
Engine::Engine(const EngineConfig& config) {
  // Configure Logger and Assets
  Logger logger_(config.loggerConfig.level);
  assets::setInstallRoot(config.assetsConfig.installRoot);
  assets::setSearchPaths(config.assetsConfig.searchPaths);

  // Initialise Engine
  spdlog::stopwatch stopWatch;
  impl_ = std::make_unique<Impl>(config);
  spdlog::info("Engine initialised successfully in {:.2}s", stopWatch);
}

Engine::~Engine() {
  spdlog::info("Engine stopping...");
};

void Engine::run() {
  spdlog::info("Engine running...");
  impl_->run();
}

void Engine::registerSceneFactory(const std::string& name,
                                  std::function<std::unique_ptr<Scene>()> factory) {
  impl_->registerSceneFactory(name, std::move(factory));
}

} // namespace blkhurst
