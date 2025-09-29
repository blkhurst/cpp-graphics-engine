#include <blkhurst/loaders/texture_loader.hpp>
#include <blkhurst/textures/texture.hpp>
#include <blkhurst/util/assets.hpp>

#include <spdlog/spdlog.h>
// Ensure STB define in only one source file
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace blkhurst {

[[nodiscard]] bool LoadedPixels::valid() const {
  return ((bytes != nullptr) || (floats != nullptr)) && width > 0 && height > 0 && channels > 0;
}

void LoadedPixels::free() {
  if (bytes != nullptr) {
    stbi_image_free(bytes);
    bytes = nullptr;
  }
  if (floats != nullptr) {
    stbi_image_free(floats);
    floats = nullptr;
  }
}

LoadedPixels TextureLoader::readPixels(const std::string& absPath, bool flipY,
                                       int desiredChannels) {
  LoadedPixels out{};

  stbi_set_flip_vertically_on_load(flipY ? 1 : 0);

  int width = 0;
  int height = 0;
  int nChannels = 0;
  const bool isHdr = stbi_is_hdr(absPath.c_str()) != 0;

  if (isHdr) {
    out.floats = stbi_loadf(absPath.c_str(), &width, &height, &nChannels, desiredChannels);
    out.isFloat = true;
  } else {
    out.bytes = stbi_load(absPath.c_str(), &width, &height, &nChannels, desiredChannels);
    out.isFloat = false;
  }

  if ((out.floats == nullptr && out.bytes == nullptr) || width <= 0 || height <= 0) {
    out.width = out.height = out.channels = 0;
    return out;
  }

  out.width = width;
  out.height = height;
  out.channels = (desiredChannels > 0) ? desiredChannels : nChannels;
  return out;
}

std::shared_ptr<Texture> TextureLoader::load(const std::string& path,
                                             const TextureLoaderDesc& desc) {
  // Resolve asset path
  auto resolvedPath = assets::find(path);
  if (!resolvedPath) {
    spdlog::error("TextureLoader asset not found ({})", path);
    return makeFallback_();
  }

  // Always output 4 channels (RGBA)
  const int outputChannels = 4; // TODO: Support auto channels/formats when bandwidth is a concern

  // Read Pixels
  LoadedPixels pixels = readPixels(*resolvedPath, desc.flipY, outputChannels);
  if (!pixels.valid()) {
    spdlog::error("TextureLoader failed to load ({})", *resolvedPath);
    pixels.free();
    return makeFallback_();
  }

  // Pick Format
  TextureFormat outputFormat = desc.srgb ? TextureFormat::SRGB8_ALPHA8 : TextureFormat::RGBA8;
  if (pixels.isFloat) {
    outputFormat = TextureFormat::RGBA32F; // Float
  }

  // Create Texture
  TextureDesc textureDesc{};
  textureDesc.format = outputFormat;
  textureDesc.minFilter = desc.minFilter;
  textureDesc.magFilter = desc.magFilter;
  textureDesc.wrapS = desc.wrapS;
  textureDesc.wrapT = desc.wrapT;
  textureDesc.generateMipmaps = desc.generateMipmaps;

  auto texture = Texture::create(pixels.width, pixels.height, textureDesc);

  // Set Texture Data
  if (pixels.isFloat) {
    texture->setPixels(static_cast<const void*>(pixels.floats), /*level*/ 0);
  } else {
    texture->setPixels(static_cast<const void*>(pixels.bytes), /*level*/ 0);
  }

  // Free pixels
  pixels.free();

  spdlog::debug("TextureLoader loaded '{}' ({}x{}, ch={}, hdr={}, srgb={})", *resolvedPath,
                texture->width(), texture->height(), pixels.channels, pixels.isFloat, desc.srgb);
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

} // namespace blkhurst
