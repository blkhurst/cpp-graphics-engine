#pragma once

#include <blkhurst/textures/texture.hpp>
#include <memory>
#include <string>

namespace blkhurst {

class Texture;

struct TextureLoaderDesc {
  bool srgb = false; // Linear / SRGB (ignored for HDR)
  bool flipY = true;

  TextureFilter minFilter = TextureFilter::LinearMipmapLinear;
  TextureFilter magFilter = TextureFilter::Linear;
  TextureWrap wrapS = TextureWrap::Repeat;
  TextureWrap wrapT = TextureWrap::Repeat;
  bool generateMipmaps = true;
};

struct LoadedPixels {
  int width = 0;
  int height = 0;
  int channels = 0;
  bool isFloat = false;
  unsigned char* bytes = nullptr; // LDR
  float* floats = nullptr;        // HDR

  void free();
  [[nodiscard]] bool valid() const;
};

struct TextureLoader {
public:
  static std::shared_ptr<Texture> load(const std::string& path, const TextureLoaderDesc& desc = {});

  // If desiredChannels is 0, file channels are used.
  static LoadedPixels readPixels(const std::string& absPath, bool flipY, int desiredChannels);

private:
  // Create a 2×2 fallback checker.
  static std::shared_ptr<Texture> makeFallback_();
};

} // namespace blkhurst
