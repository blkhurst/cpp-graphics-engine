#pragma once

#include <blkhurst/loaders/texture_loader.hpp>
#include <blkhurst/textures/cube_texture.hpp>

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace blkhurst {

inline constexpr std::size_t kCubeFaceCount = 6; // +X,-X,+Y,-Y,+Z,-Z

struct CubeTextureLoaderDesc {
  bool srgb = false;
  bool flipY = false;

  TextureFilter minFilter = TextureFilter::LinearMipmapLinear;
  TextureFilter magFilter = TextureFilter::Linear;
  TextureWrap wrapS = TextureWrap::ClampToEdge;
  TextureWrap wrapT = TextureWrap::ClampToEdge;
  // TextureWrap wrapR = TextureWrap::ClampToEdge;
  bool generateMipmaps = true;
};

struct CubeTextureLoader {
  static std::shared_ptr<CubeTexture> load(const std::array<std::string, kCubeFaceCount>& paths,
                                           const CubeTextureLoaderDesc& desc = {});

private:
  static std::shared_ptr<CubeTexture> makeFallback_();
  static bool validateFaces_(const std::vector<LoadedPixels>& faces);
};

} // namespace blkhurst
