#include <blkhurst/renderer/render_target.hpp>

#include <glad/gl.h>
#include <spdlog/spdlog.h>

namespace blkhurst {

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RenderTarget::RenderTarget(int width, int height, const RenderTargetDesc& desc)
    : width_(width),
      height_(height),
      desc_(desc) {
  glCreateFramebuffers(1, &framebufferId_);
  rebuildAttachments_();
}

RenderTarget::~RenderTarget() {
  if (framebufferId_ != 0U) {
    glDeleteFramebuffers(1, &framebufferId_);
    spdlog::trace("RenderTarget({}) destroyed", framebufferId_);
    framebufferId_ = 0U;
  }
}

std::shared_ptr<RenderTarget> RenderTarget::create(int width, int height,
                                                   const RenderTargetDesc& desc) {
  return std::make_shared<RenderTarget>(width, height, desc);
}

void RenderTarget::setSize(int width, int height) {
  const int newW = std::max(1, width);
  const int newH = std::max(1, height);
  if (newW == width_ && newH == height_) {
    return;
  }
  width_ = newW;
  height_ = newH;
  rebuildAttachments_();
}

unsigned RenderTarget::id() const {
  return framebufferId_;
}

int RenderTarget::width() const {
  return width_;
}

int RenderTarget::height() const {
  return height_;
}

std::shared_ptr<Texture> RenderTarget::texture() const {
  if (textures_.empty()) {
    spdlog::warn("RenderTarget({}) has no color attachments", framebufferId_);
    return nullptr;
  }
  return textures_[0];
}

std::shared_ptr<Texture> RenderTarget::depthTexture() const {
  return depthTexture_;
}

const std::vector<std::shared_ptr<Texture>>& RenderTarget::textures() const {
  return textures_;
}

void RenderTarget::rebuildAttachments_() {
  if (framebufferId_ == 0) {
    glCreateFramebuffers(1, &framebufferId_);
  }

  // Create color attachments
  textures_.clear();
  textures_.reserve(desc_.colorAttachmentCount);
  for (int i = 0; i < desc_.colorAttachmentCount; ++i) {
    auto texture = Texture::create(width_, height_, desc_.colorDesc);
    glNamedFramebufferTexture(framebufferId_, GL_COLOR_ATTACHMENT0 + i, texture->id(), 0);
    textures_.push_back(std::move(texture));
  }

  // Set draw buffers
  if (!textures_.empty()) {
    std::vector<GLenum> buffers;
    buffers.reserve(textures_.size());
    for (int i = 0; i < static_cast<int>(textures_.size()); ++i) {
      buffers.push_back(GL_COLOR_ATTACHMENT0 + i);
    }
    glNamedFramebufferDrawBuffers(framebufferId_, static_cast<int>(buffers.size()), buffers.data());
  } else {
    glNamedFramebufferDrawBuffer(framebufferId_, GL_NONE);
    glNamedFramebufferReadBuffer(framebufferId_, GL_NONE);
  }

  // Create depth attachment
  depthTexture_.reset();
  if (desc_.depthAttachment) {
    auto depthTexture = Texture::create(width_, height_, desc_.depthDesc);

    GLenum attachment = GL_DEPTH_ATTACHMENT;
    if (Texture::isDepthStencilFormat(desc_.depthDesc.format)) {
      attachment = GL_DEPTH_STENCIL_ATTACHMENT;
    } else if (Texture::isDepthFormat(desc_.depthDesc.format)) {
      attachment = GL_DEPTH_ATTACHMENT;
    } else {
      spdlog::error("RenderTarget depthDesc.format is not a depth format");
    }

    glNamedFramebufferTexture(framebufferId_, attachment, depthTexture->id(), 0);
    depthTexture_ = std::move(depthTexture);
  }

  // Check status
  const auto status = glCheckNamedFramebufferStatus(framebufferId_, GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    spdlog::error("RenderTarget FBO incomplete after rebuild (0x{:X})", status);
  } else {
    spdlog::trace("RenderTarget({}) {}x{} created: colors={} depth={}", framebufferId_, width_,
                  height_, textures_.size(), depthTexture_ ? "yes" : "no");
  }
}

} // namespace blkhurst
