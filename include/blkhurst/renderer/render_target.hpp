#pragma once

#include <blkhurst/textures/texture.hpp>

#include <memory>
#include <vector>

namespace blkhurst {

struct RenderTargetDesc {
  int width = 1;
  int height = 1;

  int colorAttachments = 1;
  TextureFormat colorFormat = TextureFormat::RGBA8;

  bool hasDepth = true;
  TextureFormat depthFormat = TextureFormat::D24S8;
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

  void bind() const;
  static void bindDefault();
  void setSize(int width, int height);

  [[nodiscard]] unsigned fbo() const;
  [[nodiscard]] int width() const;
  [[nodiscard]] int height() const;

  [[nodiscard]] const std::vector<std::shared_ptr<Texture>>& colorAttachments() const;
  [[nodiscard]] std::shared_ptr<Texture> depthAttachment() const;

private:
  unsigned fbo_ = 0U;
  int width_ = 0;
  int height_ = 0;

  RenderTargetDesc desc_{};
  std::vector<std::shared_ptr<Texture>> color_;
  std::shared_ptr<Texture> depth_;

  void rebuildAttachments_();
  void destroy_();
};

} // namespace blkhurst
