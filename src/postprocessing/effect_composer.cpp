#include <blkhurst/postprocessing/effect_composer.hpp>
#include <blkhurst/postprocessing/pass.hpp>
#include <blkhurst/renderer/render_target.hpp>
#include <blkhurst/renderer/renderer.hpp>

#include <algorithm>
#include <spdlog/spdlog.h>

namespace blkhurst {

EffectComposer::EffectComposer(Renderer* renderer)
    : renderer_(renderer) {
  // setSize must be called after construction to allocate buffers
  assert(renderer_ && "EffectComposer requires a non-null Renderer*");
  spdlog::debug("EffectComposer constructed");
}

EffectComposer::~EffectComposer() {
  spdlog::debug("EffectComposer destructed");
}

void EffectComposer::render() {
  if (!readBuffer_ || !writeBuffer_) {
    spdlog::error("EffectComposer::render() called before buffers were allocated.");
    return;
  }

  // Configure Renderer (RAII)
  RendererConfigurationScope configScope(renderer_);

  // Render Passes
  int idx = -1;
  for (const auto& pass : passes_) {
    idx++;

    if (!pass->isEnabled()) {
      continue;
    }

    PassRenderContext context;
    context.renderer = renderer_;
    context.writeBuffer = writeBuffer_.get();
    context.readBuffer = readBuffer_.get();
    context.renderToScreen = isLastEnabledPass(idx);
    pass->render(context);

    if (pass->needsSwap()) {
      swapBuffers();
    }
  }
}

bool EffectComposer::hasPasses() const {
  return std::ranges::any_of(passes_, [](const auto& pass) { return pass && pass->isEnabled(); });
}

void EffectComposer::addPass(const std::shared_ptr<Pass>& pass) {
  passes_.push_back(pass);
  int effectiveWidth = static_cast<int>(static_cast<float>(width_) * scale_);
  int effectiveHeight = static_cast<int>(static_cast<float>(height_) * scale_);
  pass->setSize(effectiveWidth, effectiveHeight);
}

void EffectComposer::clearPasses() {
  passes_.clear();
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void EffectComposer::setSize(int width, int height) {
  width_ = width;
  height_ = height;

  int effectiveWidth = static_cast<int>(static_cast<float>(width) * scale_);
  int effectiveHeight = static_cast<int>(static_cast<float>(height) * scale_);

  // Create Buffers Lazily
  RenderTargetDesc rtDesc;
  rtDesc.colorDesc.format = TextureFormat::RGBA16F;
  if (!readBuffer_) {
    readBuffer_ = RenderTarget::create(effectiveWidth, effectiveHeight, rtDesc);
  }
  if (!writeBuffer_) {
    writeBuffer_ = RenderTarget::create(effectiveWidth, effectiveHeight, rtDesc);
  }

  // Resize Buffers & Passes
  readBuffer_->setSize(effectiveWidth, effectiveHeight);
  writeBuffer_->setSize(effectiveWidth, effectiveHeight);

  for (const auto& pass : passes_) {
    pass->setSize(effectiveWidth, effectiveHeight);
  }
}

void EffectComposer::setScale(float scale) {
  scale_ = scale;
  setSize(width_, height_);
}

void EffectComposer::reset() {
  auto framebufferSize = renderer_->desc().framebufferSize;
  scale_ = 1.0F; // renderer_->desc().pixelRatio;
  width_ = framebufferSize[0];
  height_ = framebufferSize[1];

  setSize(width_, height_);
}

bool EffectComposer::isLastEnabledPass(int index) const {
  for (int i = index + 1; i < passes_.size(); i++) {
    if (passes_[i]->isEnabled()) {
      return false;
    }
  }
  return true;
}

void EffectComposer::swapBuffers() {
  std::swap(writeBuffer_, readBuffer_);
}

// ---------- RendererConfigurationScope ----------

RendererConfigurationScope::RendererConfigurationScope(Renderer* renderer)
    : renderer_(renderer),
      state_(renderer->desc()) {
  // EffectComposer Requires Linear Color Space & No Tone Mapping; No Need To Set Here Since
  // Renderer Only Applies OutputColorSpace/ToneMappingMode When Rendering To Default Framebuffer
}

RendererConfigurationScope::~RendererConfigurationScope() {
  // Restore Render Target
  renderer_->setRenderTarget(state_.currentTarget_);
}

} // namespace blkhurst
