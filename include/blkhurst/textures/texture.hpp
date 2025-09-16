#pragma once

#include <cstdint>
#include <memory>

namespace blkhurst {

enum class TextureFormat : std::uint8_t {
  RGBA8,
  RGBA16F,
  RGBA32F,
  R8,
  R16F,
  R32F,
  D24S8,
  D32F,
  SRGB8,
  SRGB8A8
};
enum class TextureFilter : std::uint8_t { Nearest, Linear, LinearMipmapLinear };
enum class TextureWrap : std::uint8_t { ClampToEdge, Repeat, MirroredRepeat };

struct TextureDesc {
  TextureFormat format = TextureFormat::RGBA8;
  TextureFilter minFilter = TextureFilter::LinearMipmapLinear;
  TextureFilter magFilter = TextureFilter::Linear;
  TextureWrap wrapS = TextureWrap::Repeat;
  TextureWrap wrapT = TextureWrap::Repeat;
  bool generateMipmaps = true;
};

class Texture {
public:
  Texture();
  Texture(int width, int height, const TextureDesc& desc);
  virtual ~Texture();

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;
  Texture(Texture&&) = delete;
  Texture& operator=(Texture&&) = delete;

  static std::shared_ptr<Texture> create(int width, int height, const TextureDesc& desc);

  // Sampler parameters
  void setFiltering(TextureFilter minFilter, TextureFilter magFilter) const;
  void setWrap(TextureWrap wrapS, TextureWrap wrapT) const;

  // Bind to texture unit
  void bindUnit(int unit) const;
  void setPixels(const void* pixels, int level = 0);

  [[nodiscard]] unsigned id() const;
  [[nodiscard]] int width() const;
  [[nodiscard]] int height() const;
  [[nodiscard]] TextureFormat format() const;

private:
  unsigned id_ = 0U;
  int width_ = 0;
  int height_ = 0;
  int mipLevels_ = 1;
  TextureFormat format_ = TextureFormat::RGBA8;
  TextureDesc desc_{};

  void destroy_();

  static unsigned toGLInternal(TextureFormat format);
  static unsigned toGLFilter(TextureFilter filter, bool isMin);
  static unsigned toGLWrap(TextureWrap wrap);
  static void pixelFormatAndType(TextureFormat format, unsigned& outFormat, unsigned& outType);
};

} // namespace blkhurst
