#pragma once
#include <blkhurst/postprocessing/pass.hpp>
#include <blkhurst/renderer/renderer.hpp>

#include <memory>
#include <vector>

namespace blkhurst {

class EffectComposer {
public:
  EffectComposer(Renderer* renderer);
  ~EffectComposer();

  EffectComposer(const EffectComposer&) = delete;
  EffectComposer& operator=(const EffectComposer&) = delete;
  EffectComposer(EffectComposer&&) = delete;
  EffectComposer& operator=(EffectComposer&&) = delete;

  void render();

  [[nodiscard]] bool hasPasses() const;
  void addPass(const std::shared_ptr<Pass>& pass);
  void clearPasses();

  void setSize(int width, int height);
  void setScale(float scale); // Rename setPixelRatio(float dpr);
  void reset();

private:
  Renderer* renderer_;

  // Framebuffer Size (Post-DPR)
  // TODO: Support DPR; Rename scale_ to dpr_
  int width_ = 1;
  int height_ = 1;
  float scale_ = 1.0F; // [0,1]

  // Ping-Pong Buffers & Passes
  std::shared_ptr<RenderTarget> readBuffer_;
  std::shared_ptr<RenderTarget> writeBuffer_;
  std::vector<std::shared_ptr<Pass>> passes_;

  [[nodiscard]] bool isLastEnabledPass(int index) const;
  void swapBuffers();
};

// RendererConfigurationScope - RAII to save/restore renderer state
struct RendererConfigurationScope {
  RendererConfigurationScope(Renderer* renderer);
  ~RendererConfigurationScope();

  RendererConfigurationScope(const RendererConfigurationScope&) = delete;
  RendererConfigurationScope& operator=(const RendererConfigurationScope&) = delete;
  RendererConfigurationScope(RendererConfigurationScope&&) = delete;
  RendererConfigurationScope& operator=(RendererConfigurationScope&&) = delete;

private:
  Renderer* renderer_;
  RendererDesc state_;
};

} // namespace blkhurst
