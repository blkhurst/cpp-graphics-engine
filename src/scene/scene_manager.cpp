#include "scene/scene_manager.hpp"
#include <spdlog/spdlog.h>

namespace {
constexpr bool kEagerLoadScenes = false;
constexpr int kNoActiveIndex = -1;
}; // namespace

namespace blkhurst {

void SceneManager::registerFactory(const std::string& name,
                                   std::function<std::unique_ptr<Scene>()> factory) {
  SceneEntry entry;
  entry.name = name;
  entry.factory = std::move(factory);

  const int index = static_cast<int>(sceneEntries_.size());
  sceneEntries_.push_back(std::move(entry));

  if (kEagerLoadScenes) {
    ensureConstructed(index);
  }

  // Make first registered scene active
  if (currentIndex_ == kNoActiveIndex) {
    setScene(index);
  }
}

void SceneManager::setScene(const std::string& name) {
  const int idx = indexOf(name);
  if (idx == kNoActiveIndex) {
    spdlog::warn("SceneManager: setScene({}) not found", name);
    return;
  }
  setScene(idx);
}

void SceneManager::setScene(int index) {
  if (index < 0 || index >= sceneEntries_.size()) {
    spdlog::warn("SceneManager: setScene index out of range {}", index);
    return;
  }
  ensureConstructed(index);
  currentIndex_ = index;
  spdlog::info("SceneManager: setScene({}, {})", sceneEntries_[index].name, index);
}

void SceneManager::preload(const std::string& name) {
  const int idx = indexOf(name);
  if (idx == kNoActiveIndex) {
    return;
  }
  ensureConstructed(idx);
}

void SceneManager::unload(const std::string& name) {
  const int idx = indexOf(name);
  if (idx == kNoActiveIndex) {
    return;
  }
  if (currentIndex_ == idx) {
    spdlog::warn("SceneManager: unloading current Scene");
    currentIndex_ = kNoActiveIndex;
  }
  sceneEntries_[idx].instance.reset();
  spdlog::debug("SceneManager: unloaded Scene({})", name);
}

void SceneManager::reload(const std::string& name) {
  const int idx = indexOf(name);
  if (idx == kNoActiveIndex) {
    spdlog::warn("SceneManager: reload name not found '{}'", name);
    return;
  }

  spdlog::debug("SceneManager: reloading Scene({})", name);

  sceneEntries_[idx].instance.reset();
  ensureConstructed(idx);
}

Scene* SceneManager::currentScene() const {
  if (currentIndex_ == kNoActiveIndex) {
    return nullptr;
  }
  return sceneEntries_[currentIndex_].instance.get();
}

int SceneManager::currentIndex() const {
  return currentIndex_;
}

std::vector<std::string> SceneManager::names() const {
  std::vector<std::string> out;
  out.reserve(sceneEntries_.size());
  for (const auto& sceneEntry : sceneEntries_) {
    out.push_back(sceneEntry.name);
  }
  return out;
}

int SceneManager::indexOf(const std::string& name) const {
  for (int i = 0; i < sceneEntries_.size(); ++i) {
    if (sceneEntries_[i].name == name) {
      return i;
    }
  }
  return kNoActiveIndex;
}

void SceneManager::ensureConstructed(int index) {
  if (index < 0 || index >= sceneEntries_.size()) {
    spdlog::warn("SceneManager: ensureConstructed index out of range {}", index);
    return;
  }
  auto& sceneEntry = sceneEntries_[index];
  if (!sceneEntry.instance) {
    spdlog::debug("SceneManager: constructing Scene({})", sceneEntry.name);
    sceneEntry.instance = sceneEntry.factory();
    if (!sceneEntry.instance) {
      spdlog::error("SceneManager: failed to construct Scene({})", sceneEntry.name);
    }
  }
}

} // namespace blkhurst
