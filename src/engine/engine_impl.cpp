#include "engine/engine_impl.hpp"
#include "window/glfw_callbacks.hpp"
#include <blkhurst/events/events.hpp>

namespace blkhurst {

// PImpl
Engine::Impl::Impl(const EngineConfig& config)
    : config_(config),
      window_(config.window),
      scene_(config.loading),
      ui_(config.ui, events_, window_),
      input_(events_),
      assetLoader_(config.loading.loadMode == SceneLoadPolicy::OnDemandUnloadInactive),
      loadingManager_(config.loading),
      composer_(&renderer_) {
  // Register EventBus Subscriptions
  registerEvents();

  // Wire GLFW Callbacks to our Input System
  GlfwCallbacks::attach(window_.getWindow(), input_);

  // Trigger FramebufferResized Event; Set Renderers Default Framebuffer Size
  auto windowFramebufferSize = window_.getFramebufferResolution();
  input_.pushFramebufferSize(windowFramebufferSize.width, windowFramebufferSize.height);
}

void Engine::Impl::run() {
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
    if (auto newScene = loadingManager_.pendingSceneChange(scene_.currentIndex())) {
      if (scene_.sceneLoadPolicy() == SceneLoadPolicy::OnDemandUnloadInactive) {
        assetLoader_.cancelPendingJobs();
      }
      renderer_.resetState();
      composer_.clearPasses();
      scene_.setScene(newScene.value());
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

    // Update Loading State
    auto progress = assetLoader_.progress();
    bool loading = progress.loading;
    loadingManager_.tick(ctx.tick, loading);

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
    if (composer_.hasPasses()) {
      composer_.render();
    } else {
      renderer_.render(*ctx.currentScene, *ctx.currentCamera);
    }

    // Render Loading Screen If Needed
    loadingManager_.renderLoadingScreen(renderer_);

    // Ui
    drawUi(rootState, ctx.currentScene);

    window_.swapBuffers();
  }
}

RootState Engine::Impl::buildRootState(const ClockInfo& tick, Scene* currentScene,
                                       Camera* currentCam) {
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
      .effectComposer = &composer_,
      .currentSceneIndex = scene_.currentIndex(),
      .sceneNames = scene_.names(),
  };
  return rootState;
}

FrameUniforms Engine::Impl::buildFrameUniforms(const Input& input, const ClockInfo& tick,
                                               Camera* currentCam) {
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

SceneContext Engine::Impl::gatherSceneContext(Scene* scene) {
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

void Engine::Impl::drawUi(const RootState& rootState, Scene* currentScene) {
  ui_.beginFrame();
  ui_.drawBaseUi(rootState);
  if (currentScene != nullptr) {
    for (const auto& uiEntry : currentScene->uiEntries()) {
      ui_.draw(*uiEntry, rootState);
    }
  }
  ui_.endFrame();
}

void Engine::Impl::setScene(const std::string& name) {
  scene_.setScene(name);
}

void Engine::Impl::registerSceneFactory(const std::string& name,
                                        std::function<std::unique_ptr<Scene>()> factory) {
  scene_.registerFactory(name, std::move(factory));
}

// Private
void Engine::Impl::registerEvents() {
  using namespace events;
  on<SceneChange>([this](const SceneChange& scene) {
    if (scene.index == scene_.currentIndex()) {
      return;
    }
    const bool targetAlreadyLoaded = scene_.isConstructed(scene.index);
    loadingManager_.requestSceneChange(scene.index, targetAlreadyLoaded);
  });
  on<ToggleFullscreen>(
      [this](const ToggleFullscreen& fullscreen) { window_.useFullscreen(fullscreen.enabled); });
  on<FramebufferResized>([this](const FramebufferResized& size) {
    renderer_.setDefaultFramebufferSize(size.width, size.height);
    composer_.setSize(size.width, size.height);
  });
}

} // namespace blkhurst
