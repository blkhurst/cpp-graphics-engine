#pragma once
#include <blkhurst/textures/texture.hpp>
#include <memory>
#include <string>

namespace blkhurst {

// Always output 4 channels (RGBA).
// TODO: Support auto channels/formats when bandwidth is a concern
static constexpr int kOutputChannels = 4;

struct TextureLoaderDesc {
  bool srgb = false; // Linear / SRGB (ignored for HDR)
  bool flipY = true;

  TextureFilter minFilter = TextureFilter::LinearMipmapLinear;
  TextureFilter magFilter = TextureFilter::Linear;
  TextureWrap wrapS = TextureWrap::Repeat;
  TextureWrap wrapT = TextureWrap::Repeat;
  bool generateMipmaps = true;
};

struct DecodedPixels {
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
  static std::shared_ptr<Texture> loadFromMemory(const unsigned char* data, size_t byteCount,
                                                 const TextureLoaderDesc& desc);
  static std::shared_ptr<Texture> loadFromRgba8(int width, int height, const unsigned char* rgba,
                                                const TextureLoaderDesc& desc);

  // If desiredChannels is 0, file channels are used.
  static DecodedPixels decodeFromPath(const std::string& absPath, bool flipY, int desiredChannels);
  static DecodedPixels decodeFromMemory(const unsigned char* data, size_t byteCount, bool flipY,
                                        int desiredChannels);

  // ---------- helpers ----------
  static std::shared_ptr<Texture> makeFallback(bool srgb = false);
  static TextureFormat chooseFormat(bool isFloat, bool srgb);
};

} // namespace blkhurst
