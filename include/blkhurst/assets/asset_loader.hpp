#pragma once
#include <blkhurst/assets/thread_dispatcher.hpp>
#include <blkhurst/loaders/model_loader.hpp>
#include <blkhurst/loaders/texture_loader.hpp>
#include <blkhurst/model/model_processor.hpp>
#include <blkhurst/objects/group.hpp>
#include <blkhurst/textures/texture.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace blkhurst {

/**
Notes:
- Tested thoroughly with rapid scene switching... Issues only found when exit during load:
  - Fixed crash from shared_ptr<Texture> dtor running on thread (segfault) by using custom deleter.
  - Fixed hang due to invokeMainAndWait waiting for main thread flushMainQueue during shutdown by
      adding Timeout.
  - Fixes can be reverted to simpler implementations once all GL logic is moved into Renderer.
*/

struct Progress {
  bool loading = true;
  std::string currentItem;
};

class AssetLoader {
public:
  AssetLoader(bool discardModelOnEpochChange = false);
  ~AssetLoader();

  AssetLoader(const AssetLoader&) = delete;
  AssetLoader(AssetLoader&&) = delete;
  AssetLoader& operator=(const AssetLoader&) = delete;
  AssetLoader& operator=(AssetLoader&&) = delete;

  void flushMainQueue();
  void cancelPendingJobs();
  [[nodiscard]] Progress progress();

  // ---------- Loaders ----------

  // Callback Types
  using Callback = std::function<void()>;
  using TextureCallback = std::function<void(const std::shared_ptr<Texture>& texture)>;

  // Load Texture
  std::shared_ptr<Texture> loadTexture(const std::string& path, const TextureLoaderDesc& desc = {},
                                       TextureCallback onComplete = nullptr);

  //* Imperative data is copied / moved so async safe when Assimp exits before load completes
  std::shared_ptr<Texture> loadTextureFromMemory(std::vector<uint8_t> data,
                                                 const TextureLoaderDesc& desc = {},
                                                 const std::string& label = "<memory>");

  // Load Texture from RGBA8
  std::shared_ptr<Texture> loadTextureFromRgba8(int width, int height, std::vector<uint8_t> rgba,
                                                const TextureLoaderDesc& desc = {},
                                                const std::string& label = "<rgba8>");

  // Load Model
  std::unique_ptr<Group> loadModel(const std::string& path, const ModelProcessorDesc& desc = {},
                                   Callback onComplete = nullptr);

private:
  ThreadDispatcher dispatcher_{0}; // 0 = Auto-detect worker count
  std::atomic<bool> shuttingDown_{false};

  // Epoch (Used as a temporary patch for loadModel not using weak_ptr to check validity)
  std::atomic<uint64_t> epoch_{0};
  bool discardModelOnEpochChange_;

  // Utilities
  std::shared_ptr<Texture> makeThreadSafeFallbackTexture(bool srgb);
  static void uploadDecodedToTexture(const std::shared_ptr<Texture>& texture,
                                     const DecodedPixels& pixels, const TextureLoaderDesc& desc);
  void queueAsync(std::string label, std::function<void()> func);
  void queueMain(std::string label, std::function<void()> func);
};

} // namespace blkhurst
