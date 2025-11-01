#include "engine/clock.hpp"
#include "logging/logger.hpp"
#include "scene/scene_manager.hpp"
#include "ui/ui_manager.hpp"
#include "window/glfw_callbacks.hpp"
#include "window/window_manager.hpp"
#include <blkhurst/assets/asset_loader.hpp>
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
        scene_(cfg.scenesConfig),
        ui_(cfg.uiConfig, events_, window_),
        input_(events_),
        assetLoader_(cfg.scenesConfig.loadMode == SceneLoadPolicy::OnDemandUnloadInactive) {
    // Register EventBus Subscriptions
    registerEvents();

    // Wire GLFW Callbacks to our Input System
    GlfwCallbacks::attach(window_.getWindow(), input_);

    // Trigger FramebufferResized Event; Set Renderers Default Framebuffer Size
    auto windowFramebufferSize = window_.getFramebufferResolution();
    input_.pushFramebufferSize(windowFramebufferSize.width, windowFramebufferSize.height);
  }

  void run() {
    // Preload Scenes If Instance Exists
    for (const auto& sceneEntry : scene_.sceneEntries()) {
      if (sceneEntry.instance) {
        SceneContext state = gatherSceneContext(sceneEntry.instance.get());
        RootState rootState = buildRootState(state.tick, state.currentScene, state.currentCamera);
        sceneEntry.instance->ensureStarted(rootState);
      }
    }

    // Scene Tracking
    UUID previousSceneId = kInvalidUUID;
    Scene* previousScene = nullptr;
    std::string previousSceneName;

    // Main Loop
    while (!window_.shouldClose()) {
      // Poll Events & Input
      input_.beginFrame();
      window_.pollEvents();
      input_.endFrame();
      assetLoader_.flushMainQueue();

      // Apply Pending Scene Change
      int pendingScene = pendingSceneChange_.exchange(-1);
      if (pendingScene != -1 && pendingScene != scene_.currentIndex()) {
        if (scene_.sceneLoadPolicy() == SceneLoadPolicy::OnDemandUnloadInactive) {
          assetLoader_.cancelPendingJobs();
        }
        renderer_.resetState();
        scene_.setScene(pendingScene);
      }

      // Gather Frame State
      const auto ctx = gatherSceneContext(scene_.currentScene());
      auto rootState = buildRootState(ctx.tick, ctx.currentScene, ctx.currentCamera);

      // Handle Scene Attach/Detach
      if (ctx.currentSceneId != previousSceneId) {
        if (previousScene != nullptr) {
          previousScene->onDetach();
          scene_.unloadIfNeeded(previousSceneName);
        }
        if (ctx.currentScene != nullptr) {
          ctx.currentScene->ensureStarted(rootState);
          ctx.currentScene->onAttach(rootState);
        }
        previousScene = ctx.currentScene;
        previousSceneId = ctx.currentSceneId;
        previousSceneName = scene_.currentName();
      }

      // UI only if no active scene/camera
      if ((ctx.currentScene == nullptr) || (ctx.currentCamera == nullptr)) {
        renderer_.clear();
        drawUi(rootState, ctx.currentScene);
        window_.swapBuffers();
        continue;
      }

      // Update Controller
      if (ctx.currentController != nullptr) {
        ctx.currentController->update(rootState);
      }

      // Update Camera (PerspectiveCamera calls updateAspectFromState)
      ctx.currentCamera->onUpdate(rootState);

      // Build/Set Uniforms
      auto frameUniforms = buildFrameUniforms(input_, ctx.tick, ctx.currentCamera);
      renderer_.setFrameUniforms(frameUniforms);

      // Update Scene (May call renderer.render)
      ctx.currentScene->traverse([&](Object3D& node) { node.onUpdate(rootState); });

      // Render
      renderer_.render(*ctx.currentScene, *ctx.currentCamera);

      // Ui
      drawUi(rootState, ctx.currentScene);

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
        .assets = &assetLoader_,
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
      frameUniforms.uCameraPos = currentCam->worldPosition();
      frameUniforms.uIsOrthographic = currentCam->isOrthographic() ? 1 : 0;
    }
    return frameUniforms;
  }

  struct SceneContext {
    ClockInfo tick{};
    Scene* currentScene = nullptr;
    UUID currentSceneId = kInvalidUUID;
    Camera* currentCamera = nullptr;
    Controller* currentController = nullptr;
  };

  SceneContext gatherSceneContext(Scene* scene) {
    SceneContext state;
    state.tick = clock_.tick();
    state.currentScene = scene;
    if (state.currentScene != nullptr) {
      state.currentSceneId = state.currentScene->uuid();
      state.currentCamera = state.currentScene->activeCamera();
      state.currentController = state.currentScene->activeController();
    }
    return state;
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

  void setScene(const std::string& name) {
    scene_.setScene(name);
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
  AssetLoader assetLoader_;

  std::vector<Subscription> subscriptions_;
  std::atomic<int> pendingSceneChange_{-1};

  void registerEvents() {
    using namespace events;
    on<SceneChange>([this](const SceneChange& scene) {
      pendingSceneChange_.store(scene.index, std::memory_order_relaxed);
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

void Engine::setScene(const std::string& name) {
  impl_->setScene(name);
}

void Engine::registerSceneFactory(const std::string& name,
                                  std::function<std::unique_ptr<Scene>()> factory) {
  impl_->registerSceneFactory(name, std::move(factory));
}

} // namespace blkhurst
