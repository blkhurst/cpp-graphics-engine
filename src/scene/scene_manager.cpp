#include "scene/scene_manager.hpp"
#include <spdlog/spdlog.h>

namespace blkhurst {

SceneManager::SceneManager(const ScenesConfig& config)
    : config_(config) {
}

void SceneManager::registerFactory(const std::string& name,
                                   std::function<std::unique_ptr<Scene>()> factory) {
  SceneEntry entry;
  entry.name = name;
  entry.factory = std::move(factory);

  const int index = static_cast<int>(sceneEntries_.size());
  sceneEntries_.push_back(std::move(entry));

  if (config_.loadMode == SceneLoadPolicy::Preload) {
    ensureConstructed(index);
  }
}

void SceneManager::setScene(const std::string& name) {
  const int idx = indexOf(name);
  if (idx == kNoActiveSceneIndex) {
    spdlog::warn("SceneManager Scene({}) not found", name);
    return;
  }
  setScene(idx);
}

void SceneManager::setScene(int index) {
  if (index < 0 || index >= sceneEntries_.size()) {
    spdlog::warn("SceneManager setScene index({}) out of range", index);
    return;
  }
  ensureConstructed(index);
  currentIndex_ = index;
  spdlog::info("SceneManager setScene({})", sceneEntries_[index].name);
}

void SceneManager::preload(const std::string& name) {
  const int idx = indexOf(name);
  if (idx == kNoActiveSceneIndex) {
    return;
  }
  ensureConstructed(idx);
}

void SceneManager::unload(const std::string& name) {
  const int idx = indexOf(name);
  if (idx == kNoActiveSceneIndex) {
    return;
  }
  if (currentIndex_ == idx) {
    currentIndex_ = kNoActiveSceneIndex;
  }
  sceneEntries_[idx].instance.reset();
  spdlog::debug("SceneManager unloaded Scene({})", name);
}

void SceneManager::unloadIfNeeded(const std::string& name) {
  if (config_.loadMode != SceneLoadPolicy::OnDemandUnloadInactive) {
    return;
  }
  unload(name);
}

void SceneManager::reload(const std::string& name) {
  const int idx = indexOf(name);
  if (idx == kNoActiveSceneIndex) {
    spdlog::warn("SceneManager reload Scene({}) not found", name);
    return;
  }

  spdlog::debug("SceneManager reloading Scene({})", name);

  sceneEntries_[idx].instance.reset();
  ensureConstructed(idx);
}

Scene* SceneManager::currentScene() const {
  if (currentIndex_ == kNoActiveSceneIndex) {
    return nullptr;
  }
  return sceneEntries_[currentIndex_].instance.get();
}

int SceneManager::currentIndex() const {
  return currentIndex_;
}

std::string SceneManager::currentName() const {
  if (currentIndex_ == kNoActiveSceneIndex) {
    return {};
  }
  return sceneEntries_[currentIndex_].name;
}

std::vector<std::string> SceneManager::names() const {
  std::vector<std::string> out;
  out.reserve(sceneEntries_.size());
  for (const auto& sceneEntry : sceneEntries_) {
    out.push_back(sceneEntry.name);
  }
  return out;
}

const std::vector<SceneEntry>& SceneManager::sceneEntries() const {
  return sceneEntries_;
}

[[nodiscard]] SceneLoadPolicy SceneManager::sceneLoadPolicy() const {
  return config_.loadMode;
}

int SceneManager::indexOf(const std::string& name) const {
  for (int i = 0; i < sceneEntries_.size(); ++i) {
    if (sceneEntries_[i].name == name) {
      return i;
    }
  }
  return kNoActiveSceneIndex;
}

void SceneManager::ensureConstructed(int index) {
  if (index < 0 || index >= sceneEntries_.size()) {
    spdlog::warn("SceneManager ensureConstructed index({}) out of range", index);
    return;
  }
  auto& sceneEntry = sceneEntries_[index];
  if (!sceneEntry.instance) {
    spdlog::debug("SceneManager constructing Scene({})", sceneEntry.name);
    sceneEntry.instance = sceneEntry.factory();
    sceneEntry.instance->setName(sceneEntry.name);
    if (!sceneEntry.instance) {
      spdlog::error("SceneManager failed to construct Scene({})", sceneEntry.name);
    }
  }
}

} // namespace blkhurst
