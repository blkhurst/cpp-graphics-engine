#pragma once

#include <blkhurst/engine/config/scenes.hpp>
#include <blkhurst/scene/scene.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace blkhurst {

constexpr int kNoActiveSceneIndex = -1;

struct SceneEntry {
  std::string name;
  std::function<std::unique_ptr<Scene>()> factory;
  std::unique_ptr<Scene> instance;
};

class SceneManager {
public:
  SceneManager(const ScenesConfig& config);
  ~SceneManager() = default;

  SceneManager(const SceneManager&) = delete;
  SceneManager& operator=(const SceneManager&) = delete;
  SceneManager(SceneManager&&) = delete;
  SceneManager& operator=(SceneManager&&) = delete;

  void registerFactory(const std::string& name, std::function<std::unique_ptr<Scene>()> factory);

  void setScene(const std::string& name);
  void setScene(int index);

  void preload(const std::string& name);
  void unload(const std::string& name);
  void unloadIfNeeded(const std::string& name);
  void reload(const std::string& name);

  [[nodiscard]] Scene* currentScene() const;
  [[nodiscard]] int currentIndex() const;
  [[nodiscard]] std::string currentName() const;

  [[nodiscard]] std::vector<std::string> names() const;
  [[nodiscard]] const std::vector<SceneEntry>& sceneEntries() const;

  [[nodiscard]] SceneLoadPolicy sceneLoadPolicy() const;

private:
  ScenesConfig config_;
  std::vector<SceneEntry> sceneEntries_;
  int currentIndex_ = kNoActiveSceneIndex;

  [[nodiscard]] int indexOf(const std::string& name) const;
  void ensureConstructed(int index);
};

} // namespace blkhurst
