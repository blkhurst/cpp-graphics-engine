#include <blkhurst/postprocessing/passes/bloom_pass.hpp>

namespace blkhurst {

BloomPass::BloomPass(const BloomPassDesc& desc)
    : desc_(desc) {
  setDownsamplingLevels(desc_.downsampleLevels);
  setUpsamplingLevels(desc_.upsampleLevels);
  setRadius(desc_.radius);
  setIntensity(desc_.intensity);
}

std::shared_ptr<BloomPass> BloomPass::create(const BloomPassDesc& desc) {
  return std::make_shared<BloomPass>(desc);
}

const BloomPassDesc& BloomPass::desc() const {
  return desc_;
}

void BloomPass::setDownsamplingLevels(int levels) {
  desc_.downsampleLevels = levels;

  // Create Downsample Mipmap Targets
  mipDownsamples_.clear();
  for (int i = 0; i < desc_.downsampleLevels; i++) {
    RenderTargetDesc rtDesc;
    rtDesc.depthAttachment = false;
    rtDesc.colorDesc.wrapS = TextureWrap::ClampToEdge; // CLAMP_TO_BORDER performed in shader
    rtDesc.colorDesc.wrapT = TextureWrap::ClampToEdge; // ClampToEdge required in upsampling
    auto renderTarget = RenderTarget::create(1, 1, rtDesc);
    mipDownsamples_.push_back(renderTarget);
  }

  // Resize Each Mip Level
  setSize(resolution_[0], resolution_[1]);
}

void BloomPass::setUpsamplingLevels(int levels) {
  desc_.upsampleLevels = levels;

  // Create Upsample Mipmap Targets
  mipUpsamples_.clear();
  for (int i = 0; i < desc_.upsampleLevels; i++) {
    RenderTargetDesc rtDesc;
    rtDesc.depthAttachment = false;
    rtDesc.colorDesc.wrapS = TextureWrap::ClampToEdge;
    rtDesc.colorDesc.wrapT = TextureWrap::ClampToEdge;
    auto renderTarget = RenderTarget::create(1, 1, rtDesc);
    mipUpsamples_.push_back(renderTarget);
  }

  // Resize Each Mip Level
  setSize(resolution_[0], resolution_[1]);
}

void BloomPass::setRadius(float radius) {
  desc_.radius = std::clamp(radius, 0.0F, 1.0F);
  upsampleMat_->setRadius(desc_.radius);
}

void BloomPass::setIntensity(float intensity) {
  desc_.intensity = std::clamp(intensity, 0.0F, 1.0F);
  mixMaterial_->setBloomIntensity(desc_.intensity);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void BloomPass::setSize(int width, int height) {
  // Store Resolution
  resolution_ = glm::ivec2(width, height);

  // Resize Each Mip Level
  int idx = -1;
  for (auto& downsampleTarget : mipDownsamples_) {
    idx++;

    // Halve Size; Set Downsample RT Size
    width = std::max(1, width / 2);
    height = std::max(1, height / 2);
    downsampleTarget->setSize(width, height);

    // Set Upsample RT Size
    if (idx < mipUpsamples_.size()) {
      mipUpsamples_[idx]->setSize(width, height);
    }
  }
}

void BloomPass::render(const PassRenderContext& context) {
  if (!isEnabled()) {
    return;
  }

  // Downsample
  auto* previousBuffer = context.readBuffer;
  for (const auto& mip : mipDownsamples_) {
    // Set DownsampleMaterial Input Size & Buffer
    downsampleMat_->setInputSize(previousBuffer->width(), previousBuffer->height());
    downsampleMat_->setInputBuffer(previousBuffer->texture());
    // CLAMP_TO_BORDER performed in shader

    // Set RenderTarget (Next Downsampled Mip)
    context.renderer->setRenderTarget(mip.get());

    // Render & Update Previous
    downsampleQuad_.render(*context.renderer);
    previousBuffer = mip.get();
  }

  // Upsample (Start 1 Level Below Smallest Downsampled Mip)
  int downsampleIdx = static_cast<int>(mipDownsamples_.size()) - 2;
  int upsampleLimit = std::min(desc_.upsampleLevels, static_cast<int>(mipDownsamples_.size()) - 1);
  for (int idx = upsampleLimit - 1; idx >= 0; idx--) {
    auto& mip = mipUpsamples_[idx];

    // Set UpsampleMaterial InputBuffer (Smaller Mip To Upsample From)
    // Set UpsampleMaterial SupportBuffer (Same Resolution As Output; For Lerp/Blend)
    upsampleMat_->setInputSize(previousBuffer->width(), previousBuffer->height());
    upsampleMat_->setInputBuffer(previousBuffer->texture());
    upsampleMat_->setSupportBuffer(mipDownsamples_[downsampleIdx]->texture());
    // Both Sampled Buffers are CLAMP_TO_EDGE

    //? spdlog::info("Upsample {} using Downsample Mip {}", idx, downsampleIdx);
    downsampleIdx--;

    // Set RenderTarget (Output Mip)
    context.renderer->setRenderTarget(mip.get());

    // Render & Update Previous
    upsampleQuad_.render(*context.renderer);
    previousBuffer = mip.get();
  }

  // Set RenderTarget & Mix Bloom With Original Scene
  context.renderToScreen ? context.renderer->setRenderTarget(nullptr)
                         : context.renderer->setRenderTarget(context.writeBuffer);

  mixMaterial_->setInputBuffer(context.readBuffer->texture());
  mixMaterial_->setBloomBuffer(previousBuffer->texture());
  mixQuad_.render(*context.renderer);
}

} // namespace blkhurst
