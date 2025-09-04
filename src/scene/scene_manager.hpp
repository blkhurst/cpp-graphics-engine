#pragma once

#include <blkhurst/scene/scene.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace blkhurst {

class SceneManager {
public:
  SceneManager() = default;
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
  void reload(const std::string& name);

  [[nodiscard]] Scene* currentScene() const;
  [[nodiscard]] int currentIndex() const;
  [[nodiscard]] std::vector<std::string> names() const;

private:
  struct SceneEntry {
    std::string name;
    std::function<std::unique_ptr<Scene>()> factory;
    std::unique_ptr<Scene> instance;
  };

  [[nodiscard]] int indexOf(const std::string& name) const;
  void ensureConstructed(int index);

  std::vector<SceneEntry> sceneEntries_;
  int currentIndex_ = -1;
};

} // namespace blkhurst
