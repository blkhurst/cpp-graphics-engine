#include "blkhurst/textures/cube_texture.hpp"
#include <blkhurst/scene/scene.hpp>

#include <spdlog/spdlog.h>
#include <utility>

namespace blkhurst {

Scene::Scene() {
  spdlog::trace("Scene({}) constructed", uuid());
}

Scene::~Scene() {
  spdlog::trace("Scene({}) destroyed", uuid());
}

const SceneBackground& Scene::background() const {
  return background_;
}

void Scene::setBackground(const glm::vec4& color) {
  background_.type = BackgroundType::Color;
  background_.color = color;
  background_.texture.reset();
  background_.cubemap.reset();
  spdlog::trace("Scene({}) setBackground [{:.2f}, {:.2f}, {:.2f}, {:.2f}]", uuid(), color[0],
                color[1], color[2], color[3]);
}

void Scene::setBackground(std::shared_ptr<CubeTexture> cubemap) {
  background_.type = BackgroundType::Cube;
  background_.cubemap = std::move(cubemap);
  background_.texture.reset();
  if (!background_.cubemap) {
    spdlog::warn("Scene({}) setBackground called with null CubeTexture", uuid());
    return;
  }
  spdlog::trace("Scene({}) setBackground CubeTexture({})", uuid(), background_.cubemap->id());
}

void Scene::setBackground(std::shared_ptr<Texture> equirect) {
  background_.type = BackgroundType::Equirect;
  background_.texture = std::move(equirect);
  background_.cubemap.reset();
  if (!background_.texture) {
    spdlog::warn("Scene({}) setBackground called with null Texture", uuid());
    return;
  }
  spdlog::trace("Scene({}) setBackground Texture({})", uuid(), background_.texture->id());
}

void Scene::setBackgroundIntensity(float intensity) {
  background_.intensity = intensity;
  spdlog::trace("Scene({}) setBackgroundIntensity({})", uuid(), intensity);
}

// const SceneEnvironment& Scene::environment() const {
//   return environment_;
// }

// void Scene::setEnvironment(std::shared_ptr<Texture> equirect, bool setBackground) {
//   auto hdr = std::move(equirect);
//   // environment_.cubemap = CubeTexture::fromEquirectangular(equirect);
//   // 1) Convert from Equirectangular to Cubemap
//   // 2) Generate PMREM
//   // 3) Set as environment/background
// }

// void Scene::setEnvironmentIntensity(float intensity) {
//   environment_.intensity = intensity;
//   spdlog::trace("Scene({}) setEnvironmentIntensity({})", uuid(), intensity);
// }

// void Scene::setEnvironmentRotation(const glm::mat3& rotation) {
//   environment_.rotation = rotation;
//   spdlog::trace("Scene({}) setEnvironmentRotation", uuid());
// }

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
//   spdlog::trace("Scene({}) setBackground [{:.3f}, {:.3f}, {:.3f}, {:.3f}]", uuid(),
//   background_[0],
//                 background_[1], background_[2], background_[3]);
// }

void Scene::setActiveCamera(std::shared_ptr<Camera> camera) {
  if (!camera) {
    spdlog::warn("Scene({}) setActiveCamera called with null camera", uuid());
    return;
  }
  activeCamera_ = std::move(camera);
  spdlog::trace("Scene({}) setActiveCamera({})", uuid(), activeCamera_->uuid());
}

void Scene::setActiveController(std::shared_ptr<Controller> controller) {
  if (!controller) {
    spdlog::warn("Scene({}) setActiveController called with null controller", uuid());
    return;
  }
  activeController_ = std::move(controller);
  spdlog::trace("Scene({}) setActiveController", uuid());
}

void Scene::addUiEntry(std::shared_ptr<UiEntry> entry) {
  if (!entry) {
    spdlog::warn("Scene({}) addUiEntry called with null entry", uuid());
    return;
  }
  spdlog::trace("Scene({}) addUiEntry '{}'", uuid(), entry->title());
  uiEntries_.push_back(std::move(entry));
}

} // namespace blkhurst
