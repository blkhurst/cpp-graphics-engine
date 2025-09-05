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

glm::vec4 Scene::background() const {
  return background_;
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

void Scene::setBackground(const glm::vec4& backgroundVariant) {
  background_ = backgroundVariant;
  spdlog::trace("Scene({}) setBackground [{:.3f}, {:.3f}, {:.3f}, {:.3f}]", uuid(), background_[0],
                background_[1], background_[2], background_[3]);
}

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
