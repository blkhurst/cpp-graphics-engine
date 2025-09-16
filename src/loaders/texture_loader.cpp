#include <blkhurst/loaders/texture_loader.hpp>
#include <blkhurst/textures/texture.hpp>
#include <blkhurst/util/assets.hpp>

#include <spdlog/spdlog.h>
// Ensure STB define in only one source file
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace blkhurst {

std::shared_ptr<Texture> TextureLoader::load(const std::string& path,
                                             const TextureLoaderDesc& desc) {
  // Find Asset
  auto resolvedPath = assets::find(path);
  if (!resolvedPath) {
    spdlog::error("TextureLoader asset not found ({})", path);
    return makeFallback_();
  }

  // Flip Y
  stbi_set_flip_vertically_on_load(desc.flipY ? 1 : 0);

  // Load with STB
  int width = 0;
  int height = 0;
  int nChannels = 0;
  const int nDesiredChannels = pickChannels_(/*file*/ 0, desc.desiredChannels);
  auto* pixelData = stbi_load(resolvedPath->c_str(), &width, &height, &nChannels, nDesiredChannels);
  if ((pixelData == nullptr) || width <= 0 || height <= 0) {
    spdlog::error("TextureLoader failed to load ({})", *resolvedPath);
    if (pixelData != nullptr) {
      stbi_image_free(pixelData);
    }
    return makeFallback_();
  }

  const int channels = (desc.desiredChannels == 0) ? std::max(1, nChannels) : nDesiredChannels;

  // Create Texture
  auto outDesc = desc.textureDesc;
  // TODO: if (desc.srgb) {
  outDesc.format = (channels <= 1) ? TextureFormat::R8 : TextureFormat::RGBA8;
  auto texture = Texture::create(width, height, outDesc);
  texture->setPixels(pixelData, 0);

  // Free STB & Return
  stbi_image_free(pixelData);
  spdlog::debug("TextureLoader loaded '{}' ({}x{}, ch={})", *resolvedPath, width, height, channels);
  return texture;
}

// 2×2 fallback checkerboard (magenta/black)
std::shared_ptr<Texture> TextureLoader::makeFallback_() {
  const std::array<unsigned char, 16> pixels = {255, 0, 255, 255, 0,   0, 0,   255,
                                                0,   0, 0,   255, 255, 0, 255, 255};
  TextureDesc desc{};
  desc.format = TextureFormat::RGBA8;
  desc.minFilter = TextureFilter::Nearest;
  desc.magFilter = TextureFilter::Nearest;
  desc.wrapS = TextureWrap::ClampToEdge;
  desc.wrapT = TextureWrap::ClampToEdge;

  auto texture = Texture::create(2, 2, desc);
  texture->setPixels(pixels.data(), 0);
  spdlog::warn("TextureLoader using fallback texture");
  return texture;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
int TextureLoader::pickChannels_(int fileChannels, int desiredChannels) {
  if (desiredChannels == 0) {
    return std::max(1, fileChannels);
  }
  return std::clamp(desiredChannels, 1, 4);
}

} // namespace blkhurst
