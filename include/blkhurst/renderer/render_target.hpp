#pragma once

#include <blkhurst/textures/texture.hpp>

#include <memory>
#include <vector>

namespace blkhurst {

// Mirrors TextureDesc
struct ColorAttachmentDesc {
  TextureFormat format = TextureFormat::RGBA16F;
  TextureFilter minFilter = TextureFilter::Linear;
  TextureFilter magFilter = TextureFilter::Linear;
  TextureWrap wrapS = TextureWrap::ClampToEdge;
  TextureWrap wrapT = TextureWrap::ClampToEdge;
  bool generateMipmaps = false;
};

struct DepthAttachmentDesc {
  TextureFormat format = TextureFormat::Depth32F;
  TextureFilter minFilter = TextureFilter::Nearest;
  TextureFilter magFilter = TextureFilter::Nearest;
  TextureWrap wrapS = TextureWrap::ClampToEdge;
  TextureWrap wrapT = TextureWrap::ClampToEdge;
  bool generateMipmaps = false;
};

struct RenderTargetDesc {
  int colorAttachmentCount = 1;
  ColorAttachmentDesc colorDesc;
  DepthAttachmentDesc depthDesc;

  bool depthAttachment = true;
  // TODO: samples (MSAA)
};

// Wrapper for OpenGl Framebuffer
class RenderTarget {
public:
  RenderTarget(int width, int height, const RenderTargetDesc& desc);
  ~RenderTarget();

  RenderTarget(const RenderTarget&) = delete;
  RenderTarget& operator=(const RenderTarget&) = delete;
  RenderTarget(RenderTarget&&) = delete;
  RenderTarget& operator=(RenderTarget&&) = delete;

  static std::shared_ptr<RenderTarget> create(int width, int height, const RenderTargetDesc& desc);

  void setSize(int width, int height);

  [[nodiscard]] unsigned id() const;
  [[nodiscard]] int width() const;
  [[nodiscard]] int height() const;

  [[nodiscard]] std::shared_ptr<Texture> texture() const;
  [[nodiscard]] std::shared_ptr<Texture> depthTexture() const;
  [[nodiscard]] const std::vector<std::shared_ptr<Texture>>& textures() const;

private:
  unsigned framebufferId_ = 0U;
  int width_;
  int height_;

  RenderTargetDesc desc_;
  std::vector<std::shared_ptr<Texture>> textures_;
  std::shared_ptr<Texture> depthTexture_;

  void rebuildAttachments_();
};

} // namespace blkhurst
