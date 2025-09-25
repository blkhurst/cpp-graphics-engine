#include "engine/clock.hpp"
#include "logging/logger.hpp"
#include "scene/scene_manager.hpp"
#include "ui/ui_manager.hpp"
#include "window/glfw_callbacks.hpp"
#include "window/window_manager.hpp"
#include <blkhurst/engine.hpp>
#include <blkhurst/engine/config.hpp>
#include <blkhurst/engine/root_state.hpp>
#include <blkhurst/events/event_bus.hpp>
#include <blkhurst/events/events.hpp>
#include <blkhurst/input/input.hpp>
#include <blkhurst/renderer/renderer.hpp>
#include <blkhurst/renderer/uniform_blocks.hpp>
#include <blkhurst/shaders/shader_registry.hpp>
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
        ui_(cfg.uiConfig, events_, window_),
        input_(events_) {
    // Register EventBus Subscriptions
    registerEvents();

    // Wire GLFW Callbacks to our Input System
    GlfwCallbacks::attach(window_.getWindow(), input_);

    // Trigger FramebufferResized Event; Set Renderers Default Framebuffer Size
    auto windowFramebufferSize = window_.getFramebufferResolution();
    input_.pushFramebufferSize(windowFramebufferSize.width, windowFramebufferSize.height);
  }

  void run() {
    while (!window_.shouldClose()) {
      // Poll Events & Input
      input_.beginFrame();
      window_.pollEvents();
      input_.endFrame();

      // Gather Frame State
      const auto tick = clock_.tick();
      auto* currentScene = scene_.currentScene();
      bool availableScene = (currentScene != nullptr);
      auto* currentCamera = availableScene ? currentScene->activeCamera() : nullptr;
      auto* currentController = availableScene ? currentScene->activeController() : nullptr;
      auto rootState = buildRootState(tick, currentScene, currentCamera);

      // UI only if no active scene/camera
      if ((currentScene == nullptr) || (currentCamera == nullptr)) {
        renderer_.clear();
        drawUi(rootState, currentScene);
        window_.swapBuffers();
        continue;
      }

      // Update Controller
      if (currentController != nullptr) {
        currentController->update(rootState);
      }

      // Update Camera (PerspectiveCamera calls updateAspectFromState)
      currentCamera->onUpdate(rootState);

      // Build/Set Uniforms
      auto frameUniforms = buildFrameUniforms(input_, tick, currentCamera);
      renderer_.setFrameUniforms(frameUniforms);

      // Update Scene (May call renderer.render)
      currentScene->traverse([&](Object3D& node) { node.onUpdate(rootState); });

      // Render
      renderer_.render(*currentScene, *currentCamera);

      // Ui
      drawUi(rootState, currentScene);

      window_.swapBuffers();
    }
  }

  RootState buildRootState(const ClockInfo& tick, Scene* currentScene, Camera* currentCam) {
    RootState rootState = {
        .delta = tick.delta,
        .elapsed = tick.elapsed,
        .fps = tick.fps,
        .ms = tick.ms,
        .windowFramebufferSize = input_.framebufferSize(),
        .renderer = &renderer_,
        .camera = currentCam,
        .input = &input_,
        .scene = currentScene,
        .events = &events_,
        .currentSceneIndex = scene_.currentIndex(),
        .sceneNames = scene_.names(),
    };
    return rootState;
  }

  FrameUniforms buildFrameUniforms(const Input& input, const ClockInfo& tick, Camera* currentCam) {
    FrameUniforms frameUniforms{};
    frameUniforms.uTime = tick.elapsed;
    frameUniforms.uDelta = tick.delta;
    frameUniforms.uMouse = input_.mousePosition();
    frameUniforms.uResolution = input_.framebufferSize();
    if (currentCam != nullptr) {
      frameUniforms.uView = currentCam->viewMatrix();
      frameUniforms.uProjection = currentCam->projectionMatrix();
      frameUniforms.uCameraPos = currentCam->position();
    }
    return frameUniforms;
  }

  void drawUi(const RootState& rootState, Scene* currentScene) {
    ui_.beginFrame();
    ui_.drawBaseUi(rootState);
    if (currentScene != nullptr) {
      for (const auto& uiEntry : currentScene->uiEntries()) {
        ui_.draw(*uiEntry, rootState);
      }
    }
    ui_.endFrame();
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
  Input input_;
  Renderer renderer_;

  std::vector<Subscription> subscriptions_;

  void registerEvents() {
    using namespace events;
    on<SceneChange>([this](const SceneChange& scene) {
      renderer_.resetState();
      scene_.setScene(scene.index);
    });
    on<ToggleFullscreen>(
        [this](const ToggleFullscreen& fullscreen) { window_.useFullscreen(fullscreen.enabled); });
    on<FramebufferResized>([this](const FramebufferResized& size) {
      renderer_.setDefaultFramebufferSize(size.width, size.height);
    });
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

  // Register Builtin Shaders
  ShaderRegistry::registerBuiltinShaders();

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
