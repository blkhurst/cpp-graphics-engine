#include <blkhurst/loaders/cube_texture_loader.hpp>
#include <blkhurst/util/assets.hpp>

#include <array>
#include <optional>
#include <spdlog/spdlog.h>
#include <stb_image.h>

namespace blkhurst {

std::shared_ptr<CubeTexture>
CubeTextureLoader::load(const std::array<std::string, kCubeFaceCount>& paths,
                        const CubeTextureLoaderDesc& desc) {
  // Resolve files
  std::array<std::string, kCubeFaceCount> resolved{};
  for (int i = 0; i < kCubeFaceCount; ++i) {
    auto found = assets::find(paths.at(i));
    if (!found) {
      spdlog::error("CubeTextureLoader asset not found {}", paths.at(i));
      return makeFallback_();
    }
    resolved.at(i) = *found;
  }

  // Decode each image
  CubeTextureLoaderDesc decodeDesc;
  decodeDesc.flipY = false; // Cubemaps should not be flipped
  decodeDesc.desiredChannels = desc.desiredChannels;

  std::array<std::optional<LoadedImage>, kCubeFaceCount> faces{};
  for (std::size_t i = 0; i < kCubeFaceCount; ++i) {
    faces.at(i) = decodeImage(resolved.at(i), decodeDesc);
    if (!faces.at(i).has_value()) {
      spdlog::error("CubeTextureLoader decode failed for face {} ('{}')", i, resolved.at(i));
      return makeFallback_();
    }
  }

  const int width = faces.at(0)->width;
  const int height = faces.at(0)->height;
  const int channels = faces.at(0)->channels;

  // Create cubemap and upload
  TextureDesc texDesc = desc.textureDesc;
  texDesc.format = (channels <= 1) ? TextureFormat::R8 : TextureFormat::RGBA8; // TODO: Support SRGB

  auto cube = CubeTexture::create(width, texDesc);
  for (std::size_t face = 0; face < kCubeFaceCount; ++face) {
    // order: +X, -X, +Y, -Y, +Z, -Z
    cube->setFacePixels(static_cast<int>(face), faces.at(face)->pixels.get(), 0);
  }

  spdlog::debug("CubeTextureLoader loaded cubemap ({}x{}, ch={})", width, height, channels);
  return cube;
}

std::shared_ptr<CubeTexture> CubeTextureLoader::makeFallback_() {
  // 2×2 RGBA checker (magenta/black)
  const int kSize = 2;
  constexpr auto kCount = static_cast<std::size_t>(kSize) * kSize * 4;
  const std::array<unsigned char, kCount> pixels = {255, 0, 255, 255, 0,   0, 0,   255,
                                                    0,   0, 0,   255, 255, 0, 255, 255};

  TextureDesc desc{};
  desc.format = TextureFormat::RGBA8;
  desc.minFilter = TextureFilter::Nearest;
  desc.magFilter = TextureFilter::Nearest;
  desc.wrapS = TextureWrap::ClampToEdge;
  desc.wrapT = TextureWrap::ClampToEdge;
  desc.generateMipmaps = false;

  auto cube = CubeTexture::create(kSize, desc);
  for (std::size_t i = 0; i < kCubeFaceCount; ++i) {
    cube->setFacePixels(static_cast<int>(i), pixels.data(), 0);
  }

  spdlog::warn("CubeTextureLoader using fallback texture");
  return cube;
}

std::optional<CubeTextureLoader::LoadedImage>
CubeTextureLoader::decodeImage(const std::string& path, const CubeTextureLoaderDesc& desc) {
  stbi_set_flip_vertically_on_load(desc.flipY ? 1 : 0);

  // desired channels for stb: 0 = keep file channels, otherwise clamp [1..4]
  const int stbDesired = (desc.desiredChannels == 0) ? 0 : std::clamp(desc.desiredChannels, 1, 4);

  int width = 0;
  int height = 0;
  int fileChannels = 0;
  auto* raw = stbi_load(path.c_str(), &width, &height, &fileChannels, stbDesired);
  if (raw == nullptr || width <= 0 || height <= 0) {
    spdlog::error("CubeTextureLoader::decodeImage failed ('{}')", path);
    return std::nullopt;
  }

  const int effectiveChannels = (stbDesired == 0) ? std::max(1, fileChannels) : stbDesired;

  // RAII Deleter
  auto deleter = +[](void* pixelData) {
    if (pixelData != nullptr) {
      stbi_image_free(pixelData);
    }
  };

  LoadedImage out;
  out.pixels = std::unique_ptr<unsigned char, void (*)(void*)>(raw, deleter);
  out.width = width;
  out.height = height;
  out.channels = effectiveChannels;
  return out;
}

} // namespace blkhurst
