#pragma once

#include <blkhurst/textures/texture.hpp>
#include <memory>
#include <string>

namespace blkhurst {

class Texture;

struct TextureLoaderDesc {
  bool flipY = true;
  int desiredChannels = 4; // 0 = keep file channels; default to RGBA
  TextureDesc textureDesc{};
  // TODO: Expose when engine supports SRGB -> Linear -> SRGB gamma corrected workflow
  // bool srgb = false;
};

struct TextureLoader {
public:
  static std::shared_ptr<Texture> load(const std::string& path, const TextureLoaderDesc& desc = {});

private:
  // Create a 2×2 fallback checker.
  static std::shared_ptr<Texture> makeFallback_();
  static int pickChannels_(int fileChannels, int desiredChannels);
};

} // namespace blkhurst
