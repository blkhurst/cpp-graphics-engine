#include <blkhurst/textures/texture.hpp>

#include <glad/gl.h>
#include <spdlog/spdlog.h>

namespace {
inline int calcMipLevels(int width, int height, bool enable) {
  if (!enable) {
    return 1;
  }
  const int max = std::max(width, height);
  return static_cast<int>(std::floor(std::log2(std::max(1, max)))) + 1;
}
} // namespace

namespace blkhurst {

Texture::Texture() {
  glCreateTextures(GL_TEXTURE_2D, 1, &id_);
  spdlog::trace("Texture({}) constructed (empty)", id_);
}

Texture::Texture(int width, int height, const TextureDesc& desc)
    : width_(width),
      height_(height),
      mipLevels_(calcMipLevels(width, height, desc.generateMipmaps)),
      format_(desc.format),
      desc_(desc) {
  glCreateTextures(GL_TEXTURE_2D, 1, &id_);

  const unsigned internal = toGLInternal(format_);
  glTextureStorage2D(id_, mipLevels_, internal, width_, height_);

  setFiltering(desc.minFilter, desc.magFilter);
  setWrap(desc.wrapS, desc.wrapT);

  spdlog::trace("Texture({}) {}x{} levels={} fmt={}", id_, width_, height_, mipLevels_,
                static_cast<int>(format_));
}

Texture::~Texture() {
  destroy_();
}

std::shared_ptr<Texture> Texture::create(int width, int height, const TextureDesc& desc) {
  return std::make_shared<Texture>(width, height, desc);
}

void Texture::setFiltering(TextureFilter minFilter, TextureFilter magFilter) const {
  glTextureParameteri(id_, GL_TEXTURE_MIN_FILTER, static_cast<int>(toGLFilter(minFilter, true)));
  glTextureParameteri(id_, GL_TEXTURE_MAG_FILTER, static_cast<int>(toGLFilter(magFilter, false)));
}

void Texture::setWrap(TextureWrap wrapS, TextureWrap wrapT) const {
  glTextureParameteri(id_, GL_TEXTURE_WRAP_S, static_cast<int>(toGLWrap(wrapS)));
  glTextureParameteri(id_, GL_TEXTURE_WRAP_T, static_cast<int>(toGLWrap(wrapT)));
}

void Texture::bindUnit(int unit) const {
  glBindTextureUnit(static_cast<unsigned>(unit), id_);
}

void Texture::setPixels(const void* pixels, int level) {
  if (id_ == 0U || width_ == 0 || height_ == 0) {
    spdlog::error("Texture::image called on uninitialised texture");
    return;
  }
  if (level < 0 || level >= mipLevels_) {
    spdlog::warn("Texture::image level({}) out of range [0, {})", level, mipLevels_);
    return;
  }

  unsigned srcFormat = 0;
  unsigned srcType = 0;
  pixelFormatAndType(format_, srcFormat, srcType);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTextureSubImage2D(id_, level, 0, 0, width_, height_, srcFormat, srcType, pixels);

  if (desc_.generateMipmaps && level == 0) {
    glGenerateTextureMipmap(id_);
  }
}

unsigned Texture::id() const {
  return id_;
}

int Texture::width() const {
  return width_;
}

int Texture::height() const {
  return height_;
}

TextureFormat Texture::format() const {
  return format_;
}

void Texture::destroy_() {
  if (id_ != 0U) {
    glDeleteTextures(1, &id_);
    spdlog::trace("Texture({}) destroyed", id_);
    id_ = 0U;
  }
}

/* -------------- Gl Helpers -------------- */

GLenum Texture::toGLInternal(TextureFormat format) {
  using F = TextureFormat;
  switch (format) {
  case F::RGBA8:
    return GL_RGBA8;
  case F::RGBA16F:
    return GL_RGBA16F;
  case F::RGBA32F:
    return GL_RGBA32F;
  case F::R8:
    return GL_R8;
  case F::R16F:
    return GL_R16F;
  case F::R32F:
    return GL_R32F;
  case F::D24S8:
    return GL_DEPTH24_STENCIL8;
  case F::D32F:
    return GL_DEPTH_COMPONENT32F;
  case F::SRGB8:
    return GL_SRGB8;
  case F::SRGB8A8:
    return GL_SRGB8_ALPHA8;
  }
  return GL_RGBA8;
}

GLenum Texture::toGLFilter(TextureFilter filter, bool isMin) {
  using F = TextureFilter;
  switch (filter) {
  case F::Nearest:
    return GL_NEAREST;
  case F::Linear:
    return GL_LINEAR;
  case F::LinearMipmapLinear:
    return isMin ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
  }
  return GL_LINEAR;
}

GLenum Texture::toGLWrap(TextureWrap wrap) {
  using W = TextureWrap;
  switch (wrap) {
  case W::ClampToEdge:
    return GL_CLAMP_TO_EDGE;
  case W::Repeat:
    return GL_REPEAT;
  case W::MirroredRepeat:
    return GL_MIRRORED_REPEAT;
  }
  return GL_CLAMP_TO_EDGE;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Texture::pixelFormatAndType(TextureFormat format, GLenum& outFormat, GLenum& outType) {
  using F = TextureFormat;
  switch (format) {
  case F::RGBA8:
    outFormat = GL_RGBA;
    outType = GL_UNSIGNED_BYTE;
    break;
  case F::RGBA16F:
    outFormat = GL_RGBA;
    outType = GL_HALF_FLOAT;
    break;
  case F::RGBA32F:
    outFormat = GL_RGBA;
    outType = GL_FLOAT;
    break;
  case F::R8:
    outFormat = GL_RED;
    outType = GL_UNSIGNED_BYTE;
    break;
  case F::R16F:
    outFormat = GL_RED;
    outType = GL_HALF_FLOAT;
    break;
  case F::R32F:
    outFormat = GL_RED;
    outType = GL_FLOAT;
    break;
  case F::D24S8:
    outFormat = GL_DEPTH_STENCIL;
    outType = GL_UNSIGNED_INT_24_8;
    break;
  case F::D32F:
    outFormat = GL_DEPTH_COMPONENT;
    outType = GL_FLOAT;
    break;
  case F::SRGB8:
    outFormat = GL_RGB;
    outType = GL_UNSIGNED_BYTE;
    break;
  case F::SRGB8A8:
    outFormat = GL_RGBA;
    outType = GL_UNSIGNED_BYTE;
    break;
  }
}

} // namespace blkhurst
