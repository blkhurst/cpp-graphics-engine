#include "blkhurst/textures/cube_texture.hpp"
#include <blkhurst/objects/object3d.hpp>
#include <blkhurst/scene/scene.hpp>

#include <spdlog/spdlog.h>
#include <utility>

namespace blkhurst {

Scene::Scene()
    : Object3D(NodeType::Object) {
  spdlog::trace("Scene({}) constructed", uuidString());
}

Scene::~Scene() {
  spdlog::trace("Scene({}) destroyed", uuidString());
}

void Scene::ensureStarted(const RootState& state) {
  if (!started_) {
    onStart(state);
    started_ = true;
  }
}

const SceneBackground& Scene::background() const {
  return background_;
}

void Scene::setBackground(const glm::vec4& color) {
  background_.type = BackgroundType::Color;
  background_.color = color;
  background_.texture.reset();
  background_.cubemap.reset();
  spdlog::trace("Scene({}) setBackground [{:.2f}, {:.2f}, {:.2f}, {:.2f}]", uuidString(), color[0],
                color[1], color[2], color[3]);
}

void Scene::setBackground(std::shared_ptr<CubeTexture> cubemap) {
  // Clear Background
  if (cubemap == nullptr) {
    this->setBackground(background_.color);
    spdlog::debug("Scene({}) Background cleared", uuidString());
    return;
  }
  background_.type = BackgroundType::Cube;
  background_.cubemap = std::move(cubemap);
  background_.texture.reset();
  spdlog::debug("Scene({}) setBackground CubeTexture({})", uuidString(), background_.cubemap->id());
}

void Scene::setBackground(std::shared_ptr<Texture> equirect) {
  // Clear Background
  if (equirect == nullptr) {
    this->setBackground(background_.color);
    spdlog::debug("Scene({}) Background cleared", uuidString());
    return;
  }
  background_.type = BackgroundType::Equirect;
  background_.texture = std::move(equirect);
  background_.cubemap.reset();
  spdlog::debug("Scene({}) setBackground Texture({})", uuidString(), background_.texture->id());
}

void Scene::setBackgroundIntensity(float intensity) {
  background_.intensity = intensity;
  spdlog::trace("Scene({}) setBackgroundIntensity({})", uuidString(), intensity);
}

SceneEnvironment& Scene::environment() {
  return environment_;
}

void Scene::setEnvironment(std::shared_ptr<Texture> equirect, bool setBackground) {
  // Clear environment
  if (equirect == nullptr) {
    environment_.equirect.reset();
    environment_.brdfLUT.reset();
    environment_.irradianceMap.reset();
    environment_.prefilterMap.reset();
    environment_.needsUpdate = false;
    if (setBackground) {
      this->setBackground(background_.color);
    }
    spdlog::debug("Scene({}) Environment cleared", uuidString());
    return;
  }
  environment_.equirect = std::move(equirect);
  environment_.setBackground = setBackground;
  environment_.needsUpdate = true;
  spdlog::debug("Scene({}) setEnvironment Texture({})", uuidString(), environment_.equirect->id());
}

void Scene::setEnvironmentIntensity(float intensity) {
  environment_.intensity = intensity;
  spdlog::trace("Scene({}) setEnvironmentIntensity({})", uuidString(), intensity);
}

void Scene::setEnvironmentRotation(const glm::mat3& rotation) {
  environment_.rotation = rotation;
  spdlog::trace("Scene({}) setEnvironmentRotation", uuidString());
}

Camera* Scene::activeCamera() const {
  return activeCamera_.get();
}

Controller* Scene::activeController() const {
  return activeController_.get();
}

const std::vector<std::shared_ptr<UiEntry>>& Scene::uiEntries() const {
  return uiEntries_;
}

// void Scene::setBackground(const glm::vec4& backgroundVariant) {
//   background_ = backgroundVariant;
//   spdlog::trace("Scene({}) setBackground [{:.3f}, {:.3f}, {:.3f}, {:.3f}]", uuidString(),
//   background_[0],
//                 background_[1], background_[2], background_[3]);
// }

void Scene::setActiveCamera(std::shared_ptr<Camera> camera) {
  if (!camera) {
    spdlog::warn("Scene({}) setActiveCamera called with null camera", uuidString());
    return;
  }
  activeCamera_ = std::move(camera);
  spdlog::trace("Scene({}) setActiveCamera({})", uuidString(), activeCamera_->uuidString());
}

void Scene::setActiveController(std::shared_ptr<Controller> controller) {
  if (!controller) {
    spdlog::warn("Scene({}) setActiveController called with null controller", uuidString());
    return;
  }
  activeController_ = std::move(controller);
  spdlog::trace("Scene({}) setActiveController", uuidString());
}

} // namespace blkhurst
