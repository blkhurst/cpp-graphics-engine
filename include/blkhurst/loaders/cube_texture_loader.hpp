#pragma once

#include <blkhurst/textures/cube_texture.hpp>
#include <memory>
#include <optional>

namespace blkhurst {

inline constexpr std::size_t kCubeFaceCount = 6; // +X,-X,+Y,-Y,+Z,-Z

struct CubeTextureLoaderDesc {
  bool flipY = false;
  int desiredChannels = 4;
  TextureDesc textureDesc{};
};

struct CubeTextureLoader {
  static std::shared_ptr<CubeTexture> load(const std::array<std::string, kCubeFaceCount>& paths,
                                           const CubeTextureLoaderDesc& desc = {});

private:
  struct LoadedImage {
    std::unique_ptr<unsigned char, void (*)(void*)> pixels{nullptr, nullptr};
    int width = 0;
    int height = 0;
    int channels = 0;
  };

  static std::shared_ptr<CubeTexture> makeFallback_();
  static std::optional<LoadedImage> decodeImage(const std::string& path,
                                                const CubeTextureLoaderDesc& desc);
};

} // namespace blkhurst
