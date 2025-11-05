#include <blkhurst/loaders/texture_loader.hpp>
#include <blkhurst/textures/texture.hpp>
#include <blkhurst/util/assets.hpp>

#include <spdlog/spdlog.h>
// Ensure STB define in only one source file
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace blkhurst {

// ------------------ DecodedPixels ------------------

[[nodiscard]] bool DecodedPixels::valid() const {
  return ((bytes != nullptr) || (floats != nullptr)) && width > 0 && height > 0 && channels > 0;
}

void DecodedPixels::free() {
  if (bytes != nullptr) {
    stbi_image_free(bytes);
    bytes = nullptr;
  }
  if (floats != nullptr) {
    stbi_image_free(floats);
    floats = nullptr;
  }
}

// ------------------ TextureLoader ------------------

std::shared_ptr<Texture> TextureLoader::load(const std::string& path,
                                             const TextureLoaderDesc& desc) {
  // Read Pixels
  DecodedPixels pixels = decodeFromPath(path, desc.flipY, kOutputChannels);
  if (!pixels.valid()) {
    spdlog::warn("TextureLoader failed to load ({})", path);
    pixels.free();
    return makeFallback();
  }

  // Create Texture
  TextureDesc textureDesc{};
  textureDesc.format = chooseFormat(pixels.isFloat, desc.srgb);
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

  // Free pixels & Return
  pixels.free();
  spdlog::debug("TextureLoader loaded '{}' ({}x{}, ch={}, hdr={}, srgb={})", path, pixels.width,
                pixels.height, pixels.channels, pixels.isFloat, desc.srgb);
  return texture;
}

std::shared_ptr<Texture> TextureLoader::loadFromMemory(const unsigned char* data, size_t byteCount,
                                                       const TextureLoaderDesc& desc) {
  // Read Pixels
  DecodedPixels pixels = decodeFromMemory(data, byteCount, desc.flipY, kOutputChannels);
  if (!pixels.valid()) {
    spdlog::warn("TextureLoader: failed to decode image from memory ({} bytes)", byteCount);
    pixels.free();
    return makeFallback();
  }

  // Create Texture
  TextureDesc textureDesc{};
  textureDesc.format = chooseFormat(pixels.isFloat, desc.srgb);
  textureDesc.minFilter = desc.minFilter;
  textureDesc.magFilter = desc.magFilter;
  textureDesc.wrapS = desc.wrapS;
  textureDesc.wrapT = desc.wrapT;
  textureDesc.generateMipmaps = desc.generateMipmaps;
  auto texture = Texture::create(pixels.width, pixels.height, textureDesc);

  // Set Texture Data
  if (pixels.isFloat) {
    texture->setPixels(static_cast<const void*>(pixels.floats), 0);
  } else {
    texture->setPixels(static_cast<const void*>(pixels.bytes), 0);
  }

  // Free pixels & Return
  pixels.free();
  spdlog::debug("TextureLoader loaded from memory ({}x{}, hdr={}, srgb={})", pixels.width,
                pixels.height, pixels.isFloat, desc.srgb);
  return texture;
}

std::shared_ptr<Texture> TextureLoader::loadFromRgba8(int width, int height,
                                                      const unsigned char* rgba,
                                                      const TextureLoaderDesc& desc) {
  if ((rgba == nullptr) || width <= 0 || height <= 0) {
    spdlog::warn("TextureLoader::fromRgba8 invalid args");
    return makeFallback();
  }

  // Create Texture
  TextureDesc textureDesc{};
  textureDesc.format = desc.srgb ? TextureFormat::SRGB8_ALPHA8 : TextureFormat::RGBA8;
  textureDesc.minFilter = desc.minFilter;
  textureDesc.magFilter = desc.magFilter;
  textureDesc.wrapS = desc.wrapS;
  textureDesc.wrapT = desc.wrapT;
  textureDesc.generateMipmaps = desc.generateMipmaps;
  auto texture = Texture::create(width, height, textureDesc);

  // Set Texture Data & Return
  texture->setPixels(rgba, 0);
  return texture;
}

DecodedPixels TextureLoader::decodeFromPath(const std::string& path, bool flipY,
                                            int desiredChannels) {
  DecodedPixels out{};

  // Resolve Asset Path
  auto resolvedPath = assets::find(path);
  if (!resolvedPath) {
    spdlog::warn("TextureLoader::decodeFromPath asset not found ({})", path);
    return out;
  }

  stbi_set_flip_vertically_on_load(flipY ? 1 : 0);

  int width = 0;
  int height = 0;
  int nChannels = 0;
  const bool isHdr = stbi_is_hdr(resolvedPath->c_str()) != 0;

  if (isHdr) {
    out.floats = stbi_loadf(resolvedPath->c_str(), &width, &height, &nChannels, desiredChannels);
    out.isFloat = true;
  } else {
    out.bytes = stbi_load(resolvedPath->c_str(), &width, &height, &nChannels, desiredChannels);
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

DecodedPixels TextureLoader::decodeFromMemory(const unsigned char* data, size_t byteCount,
                                              bool flipY, int desiredChannels) {
  DecodedPixels out{};
  if ((data == nullptr) || byteCount == 0) {
    return out;
  }

  stbi_set_flip_vertically_on_load(flipY ? 1 : 0);

  int width = 0;
  int height = 0;
  int nChannels = 0;
  const bool isHdr = stbi_is_hdr_from_memory(data, static_cast<int>(byteCount)) != 0;

  if (isHdr) {
    out.floats = stbi_loadf_from_memory(data, static_cast<int>(byteCount), &width, &height,
                                        &nChannels, desiredChannels);
    out.isFloat = true;
  } else {
    out.bytes = stbi_load_from_memory(data, static_cast<int>(byteCount), &width, &height,
                                      &nChannels, desiredChannels);
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

// 2×2 fallback checkerboard (magenta/black)
std::shared_ptr<Texture> TextureLoader::makeFallback(bool srgb) {
  const std::array<unsigned char, 16> pixels = {255, 0, 255, 255, 0,   0, 0,   255,
                                                0,   0, 0,   255, 255, 0, 255, 255};
  TextureDesc desc{};
  desc.format = srgb ? TextureFormat::SRGB8_ALPHA8 : TextureFormat::RGBA8;
  desc.minFilter = TextureFilter::Nearest;
  desc.magFilter = TextureFilter::Nearest;
  desc.wrapS = TextureWrap::ClampToEdge;
  desc.wrapT = TextureWrap::ClampToEdge;

  auto texture = Texture::create(2, 2, desc);
  texture->setPixels(pixels.data(), 0);
  return texture;
}

TextureFormat TextureLoader::chooseFormat(bool isFloat, bool srgb) {
  // Currently always 4 channels (RGBA)
  if (isFloat) {
    return TextureFormat::RGBA32F;
  }
  return srgb ? TextureFormat::SRGB8_ALPHA8 : TextureFormat::RGBA8;
}

} // namespace blkhurst
