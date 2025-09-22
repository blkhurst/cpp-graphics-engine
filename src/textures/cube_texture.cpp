#include <blkhurst/textures/cube_texture.hpp>
#include <blkhurst/textures/texture.hpp>

#include <cmath>
#include <glad/gl.h>
#include <spdlog/spdlog.h>

namespace blkhurst {

CubeTexture::CubeTexture(int size, const TextureDesc& desc) {
  unsigned texId = 0U;
  glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texId);

  // Allocate storage for 6 faces
  const GLenum internal = toGLInternal(desc.format);
  const int levels = calcMipLevels(size, size, desc.generateMipmaps);

  glTextureStorage2D(texId, levels, internal, size, size);

  // Send state to base class
  adoptGLTexture(texId, size, size, levels, desc);

  setFiltering(desc.minFilter, desc.magFilter);
  setWrap(desc.wrapS, desc.wrapT, desc.wrapT); // wrapR defaults to wrapT

  spdlog::trace("CubeTexture({}) {}x{} levels={} fmt={}", texId, size, size, levels,
                static_cast<int>(desc.format));
}

std::shared_ptr<CubeTexture> CubeTexture::create(int size, const TextureDesc& desc) {
  return std::make_shared<CubeTexture>(size, desc);
}

void CubeTexture::setWrap(TextureWrap wrapS, TextureWrap wrapT, TextureWrap wrapR) const {
  glTextureParameteri(id(), GL_TEXTURE_WRAP_S, static_cast<int>(toGLWrap(wrapS)));
  glTextureParameteri(id(), GL_TEXTURE_WRAP_T, static_cast<int>(toGLWrap(wrapT)));
  glTextureParameteri(id(), GL_TEXTURE_WRAP_R, static_cast<int>(toGLWrap(wrapR)));
}

void CubeTexture::setFacePixels(int faceIndex, const void* pixels, int level) {
  if (id() == 0U || width() == 0U || height() == 0U) {
    spdlog::error("CubeTexture::setFacePixels called on uninitialised texture");
    return;
  }
  if (level < 0 || level >= mipLevels()) {
    spdlog::warn("CubeTexture::setFacePixels level({}) out of range [0, {})", level, mipLevels());
    return;
  }
  const int maxFaceCount = 6;
  if (faceIndex < 0 || faceIndex >= maxFaceCount) {
    spdlog::warn("CubeTexture::setFacePixels invalid face {}", faceIndex);
    return;
  }

  unsigned srcFormat = 0;
  unsigned srcType = 0;
  pixelFormatAndType(desc().format, srcFormat, srcType);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTextureSubImage3D(id(), level, 0, 0, faceIndex, std::max(1, width() >> level),
                      std::max(1, height() >> level), 1, srcFormat, srcType, pixels);

  // TODO: Generate once all 6 faces are set?
  if (desc().generateMipmaps && level == 0) {
    glGenerateTextureMipmap(id());
  }
}

} // namespace blkhurst
