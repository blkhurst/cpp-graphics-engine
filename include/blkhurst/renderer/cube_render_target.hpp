#pragma once
#include <blkhurst/renderer/render_target.hpp>
#include <blkhurst/textures/cube_texture.hpp>

namespace blkhurst {

class Renderer;

struct CubeRenderTargetDesc {
  int colorAttachmentCount = 1;
  ColorAttachmentDesc colorDesc;
  DepthAttachmentDesc depthDesc;

  bool depthAttachment = false;
  // TODO: samples (MSAA)
};

class CubeRenderTarget {
public:
  CubeRenderTarget(int size, const CubeRenderTargetDesc& desc = {});
  ~CubeRenderTarget();

  CubeRenderTarget(const CubeRenderTarget&) = delete;
  CubeRenderTarget& operator=(const CubeRenderTarget&) = delete;
  CubeRenderTarget(CubeRenderTarget&&) = delete;
  CubeRenderTarget& operator=(CubeRenderTarget&&) = delete;

  static std::shared_ptr<CubeRenderTarget> create(int size, const CubeRenderTargetDesc& desc = {});

  [[nodiscard]] unsigned id() const;
  [[nodiscard]] int size() const;

  [[nodiscard]] std::shared_ptr<CubeTexture> texture() const;
  [[nodiscard]] std::shared_ptr<CubeTexture> depthTexture() const;

  static std::shared_ptr<CubeRenderTarget> fromEquirect(Renderer& renderer,
                                                        const std::shared_ptr<Texture>& equirect,
                                                        const CubeRenderTargetDesc& desc = {});

  void setSize(int size);

private:
  unsigned framebufferId_ = 0U;
  int size_;

  CubeRenderTargetDesc desc_;
  std::shared_ptr<CubeTexture> texture_;
  std::shared_ptr<CubeTexture> depthTexture_;

  void rebuildAttachments_();
};

} // namespace blkhurst
