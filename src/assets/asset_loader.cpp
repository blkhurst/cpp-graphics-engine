#include <blkhurst/assets/asset_loader.hpp>
#include <blkhurst/loaders/texture_loader.hpp>
#include <blkhurst/util/assets.hpp>

#include <spdlog/spdlog.h>
#include <utility>

namespace blkhurst {

AssetLoader::AssetLoader(bool discardModelOnEpochChange)
    : discardModelOnEpochChange_(discardModelOnEpochChange) {
  spdlog::debug("AssetLoader constructed");
}

AssetLoader::~AssetLoader() {
  shuttingDown_.store(true, std::memory_order_relaxed);
  cancelPendingJobs();
  // RAII handles dispatcher destruction
  spdlog::debug("AssetLoader destructed");
}

void AssetLoader::flushMainQueue() {
  dispatcher_.flushMainQueue();
}

void AssetLoader::cancelPendingJobs() {
  dispatcher_.cancelPendingJobs();
  // Temporary - Increment Epoch To Invalidate Pending Model Loads When Using OnDemandUnloadInactive
  epoch_.fetch_add(1, std::memory_order_relaxed);
}

Progress AssetLoader::progress() {
  Progress progress;
  progress.loading = !dispatcher_.isIdle();
  progress.currentItem = dispatcher_.activeWorkerLabel();
  return progress;
}

std::shared_ptr<Texture> AssetLoader::loadTexture(const std::string& path,
                                                  const TextureLoaderDesc& desc,
                                                  TextureCallback onComplete) {
  // TODO: Caution, While Texture Makes GL Calls, Must Be Run On Main Thread
  // TODO: This used to hang indefinitely on shutdown, waits for main thread to process queue
  // Create Texture On Main Thread (GL context required)
  std::shared_ptr<Texture> placeholder = makeThreadSafeFallbackTexture(desc.srgb);

  // Use weak_ptr To Check If Texture Still Exists In Callback
  auto weak = std::weak_ptr<Texture>(placeholder);
  std::string asyncLabel = "Decoding Texture: " + path;
  std::string mainLabel = "Uploading Texture: " + path;

  //
  auto threadLambda = [=, this]() mutable {
    // Read Pixels
    DecodedPixels pixels = TextureLoader::decodeFromPath(path, desc.flipY, kOutputChannels);
    if (!pixels.valid()) {
      // Failed to Load Texture (Logged in TextureLoader)
      return;
    }

    // Queue MainThread Work
    queueMain(mainLabel, [=, this, onComplete = std::move(onComplete)]() mutable {
      // If Texture Still Exists, Upload Pixels
      if (auto texture = weak.lock()) {
        uploadDecodedToTexture(texture, pixels, desc);

        // Call Completion Callback
        if (onComplete) {
          onComplete(texture);
        }
      }

      // Free Pixels
      pixels.free();
    });
  };

  queueAsync(asyncLabel, threadLambda);
  return placeholder;
}

std::shared_ptr<Texture> AssetLoader::loadTextureFromMemory(std::vector<uint8_t> data,
                                                            const TextureLoaderDesc& desc,
                                                            const std::string& label) {
  // Create Texture On Main Thread (GL context required)
  std::shared_ptr<Texture> placeholder = makeThreadSafeFallbackTexture(desc.srgb);

  // Use weak_ptr To Check If Texture Still Exists In Callback
  auto weak = std::weak_ptr<Texture>(placeholder);
  std::string asyncLabel = "Decoding Texture: " + label;
  std::string mainLabel = "Uploading Texture: " + label;

  //
  auto threadLambda = [=, this, data = std::move(data)] {
    // Read Pixels
    DecodedPixels pixels =
        TextureLoader::decodeFromMemory(data.data(), data.size(), desc.flipY, kOutputChannels);
    if (!pixels.valid()) {
      spdlog::warn("AssetLoader: failed to read pixels from memory ({} bytes)", data.size());
      pixels.free();
      return;
    }

    // Queue MainThread Work
    queueMain(mainLabel, [=, this]() mutable {
      // If Texture Still Exists, Upload Pixels
      if (auto texture = weak.lock()) {
        uploadDecodedToTexture(texture, pixels, desc);
      }

      // Free Pixels
      pixels.free();
    });
  };

  queueAsync(asyncLabel, threadLambda);
  return placeholder;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
std::shared_ptr<Texture> AssetLoader::loadTextureFromRgba8(int width, int height,
                                                           std::vector<uint8_t> rgba,
                                                           const TextureLoaderDesc& desc,
                                                           const std::string& label) {
  // Create Texture On Main Thread (GL context required)
  std::shared_ptr<Texture> placeholder = makeThreadSafeFallbackTexture(desc.srgb);

  // Use weak_ptr To Check If Texture Still Exists In Callback
  auto weak = std::weak_ptr<Texture>(placeholder);

  //
  auto mainThreadLambda = [=, this, rgba = std::move(rgba)]() mutable {
    // If Texture Still Exists, Upload Pixels
    if (auto texture = weak.lock()) {
      DecodedPixels pixels;
      pixels.width = width;
      pixels.height = height;
      pixels.channels = kOutputChannels;
      pixels.isFloat = false;
      pixels.bytes = rgba.data();

      // Upload Pixels
      uploadDecodedToTexture(texture, pixels, desc);
    }
  };

  // Queue MainThread Work
  std::string mainLabel = "Uploading Texture: " + label;
  queueMain(mainLabel, mainThreadLambda);

  return placeholder;
}

std::unique_ptr<Group> AssetLoader::loadModel(const std::string& path,
                                              const ModelProcessorDesc& desc, Callback onComplete) {
  // Create Placeholder Group (Geometry must be created on main thread)
  auto placeholder = std::make_unique<Group>();
  placeholder->setName(path);

  // TODO: Must change to weak_ptr to check validity later.
  // TODO: For now, use epoch to check if still valid.
  Group* raw = placeholder.get();
  uint64_t epoch = epoch_.load(std::memory_order_relaxed);

  //
  auto threadLambda = [=, this, onComplete = std::move(onComplete)] {
    // Load Model // TODO: Prefer passing weak_ptr to ModelProcessor to check AssetLoader validity
    ModelCPU model = ModelProcessor::load(path, desc, this);
    if (!model.success) {
      // Failed to Load Model (Logged in ModelProcessor)
      return;
    }

    // Queue MainThread Work
    std::string label = "Building Model: " + path;
    queueMain(label, [=, this, model = std::move(model)]() {
      // Check Epoch // TODO: Remove when ModelProcessor uses weak_ptr to check validity
      if (epoch != epoch_.load(std::memory_order_relaxed) && discardModelOnEpochChange_) {
        spdlog::warn("AssetLoader::loadModel epoch mismatch '{}'", path);
        return;
      }

      // Build Model Group (GL context required)
      auto built = ModelLoader::buildGroup(model.root);

      // If Target Still Exists, Attach Built Model // TODO: Change to weak_ptr check
      if (raw != nullptr) {
        spdlog::trace("AssetLoader: Attaching built model to target group");
        raw->addChild(std::move(built));
        spdlog::trace("AssetLoader: Attached built model to target group");

        // Call Completion Callback
        if (onComplete) {
          onComplete();
        }
      } else {
        spdlog::warn("Target group is null; cannot attach built model");
      }
    });
  };

  queueAsync("Loading Model: " + path, threadLambda);
  return placeholder;
}

std::shared_ptr<Texture> AssetLoader::makeThreadSafeFallbackTexture(bool srgb) {
  // Create Texture On Main Thread (GL context required)
  std::shared_ptr<Texture> placeholder;
  dispatcher_.invokeMainAndWait([&] {
    // Fallback Texture Data & Desc
    static constexpr std::array<unsigned char, 16> kPixels = {255, 0, 255, 255, 0,   0, 0,   255,
                                                              0,   0, 0,   255, 255, 0, 255, 255};
    TextureDesc desc{};
    desc.format = srgb ? TextureFormat::SRGB8_ALPHA8 : TextureFormat::RGBA8;
    desc.minFilter = TextureFilter::Nearest;
    desc.magFilter = TextureFilter::Nearest;
    desc.wrapS = TextureWrap::ClampToEdge;
    desc.wrapT = TextureWrap::ClampToEdge;

    // Custom Deleter (Ensure deletion on main thread)
    auto customDeleter = [&](Texture* texture) {
      dispatcher_.invokeMainAndWait([texture] { delete texture; });
    };

    // Create Texture with Custom Deleter
    placeholder = std::shared_ptr<Texture>(new Texture(2, 2, desc), customDeleter);
    placeholder->setPixels(kPixels.data(), 0);
  });
  return placeholder;
}

void AssetLoader::uploadDecodedToTexture(const std::shared_ptr<Texture>& texture,
                                         const DecodedPixels& pixels,
                                         const TextureLoaderDesc& desc) {
  // Recreate Texture
  TextureDesc textureDesc;
  textureDesc.format = TextureLoader::chooseFormat(pixels.isFloat, desc.srgb);
  textureDesc.minFilter = desc.minFilter;
  textureDesc.magFilter = desc.magFilter;
  textureDesc.wrapS = desc.wrapS;
  textureDesc.wrapT = desc.wrapT;
  textureDesc.generateMipmaps = desc.generateMipmaps;
  texture->recreate(pixels.width, pixels.height, textureDesc);

  // Set Texture Data & Regenerate Mipmaps
  if (pixels.isFloat) {
    texture->setPixels(static_cast<const void*>(pixels.floats), 0);
  } else {
    texture->setPixels(static_cast<const void*>(pixels.bytes), 0);
  }
}

void AssetLoader::queueAsync(std::string label, std::function<void()> func) {
  // Exit If Shutting Down; Prevents Lambdas Touching Destroyed AssetLoader Members
  if (shuttingDown_.load(std::memory_order_relaxed)) {
    return;
  }

  // Enqueue Job
  dispatcher_.enqueueAsync(std::move(func), std::move(label));
}

void AssetLoader::queueMain(std::string label, std::function<void()> func) {
  // Exit If Shutting Down; Prevents Lambdas Touching Destroyed AssetLoader Members
  if (shuttingDown_.load(std::memory_order_relaxed)) {
    return;
  }

  // Enqueue Job
  dispatcher_.enqueueMain(std::move(func), std::move(label));
}

} // namespace blkhurst
