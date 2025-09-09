#include <blkhurst/renderer/render_target.hpp>

#include <glad/gl.h>
#include <spdlog/spdlog.h>

namespace blkhurst {

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RenderTarget::RenderTarget(int width, int height, const RenderTargetDesc& desc)
    : width_(width),
      height_(height),
      desc_(desc) {
  glCreateFramebuffers(1, &fbo_);
  rebuildAttachments_();
}

RenderTarget::~RenderTarget() {
  destroy_();
}

std::shared_ptr<RenderTarget> RenderTarget::create(int width, int height,
                                                   const RenderTargetDesc& desc) {
  return std::make_shared<RenderTarget>(width, height, desc);
}

void RenderTarget::destroy_() {
  if (fbo_ != 0U) {
    glDeleteFramebuffers(1, &fbo_);
    spdlog::trace("RenderTarget({}) destroyed", fbo_);
    fbo_ = 0U;
  }
}

void RenderTarget::bind() const {
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glViewport(0, 0, width_, height_);
}

void RenderTarget::bindDefault() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  //? Renderer should resize viewport
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

unsigned RenderTarget::fbo() const {
  return fbo_;
}

int RenderTarget::width() const {
  return width_;
}

int RenderTarget::height() const {
  return height_;
}

const std::vector<std::shared_ptr<Texture>>& RenderTarget::colorAttachments() const {
  return color_;
}

std::shared_ptr<Texture> RenderTarget::depthAttachment() const {
  return depth_;
}

void RenderTarget::rebuildAttachments_() {
  // Create color attachments
  color_.clear();
  color_.reserve(desc_.colorAttachments);
  for (int i = 0; i < desc_.colorAttachments; ++i) {
    TextureDesc texDesc;
    texDesc.format = desc_.colorFormat;
    texDesc.generateMipmaps = false;
    auto tex = Texture::create(width_, height_, texDesc);
    glNamedFramebufferTexture(fbo_, GL_COLOR_ATTACHMENT0 + i, tex->id(), 0);
    color_.push_back(std::move(tex));
  }

  // Create depth attachment if requested
  depth_.reset();
  if (desc_.hasDepth) {
    TextureDesc depthDesc;
    depthDesc.format = desc_.depthFormat;
    depthDesc.generateMipmaps = false;
    auto dtex = Texture::create(width_, height_, depthDesc);
    const GLenum attach = (desc_.depthFormat == TextureFormat::D24S8) ? GL_DEPTH_STENCIL_ATTACHMENT
                                                                      : GL_DEPTH_ATTACHMENT;
    glNamedFramebufferTexture(fbo_, attach, dtex->id(), 0);
    depth_ = std::move(dtex);
  }

  // Set draw buffers
  if (!color_.empty()) {
    std::vector<GLenum> buffers;
    buffers.reserve(color_.size());
    for (int i = 0; i < static_cast<int>(color_.size()); ++i) {
      buffers.push_back(GL_COLOR_ATTACHMENT0 + i);
    }
    glNamedFramebufferDrawBuffers(fbo_, static_cast<int>(buffers.size()), buffers.data());
  } else {
    glNamedFramebufferDrawBuffer(fbo_, GL_NONE);
  }

  // Check status
  const auto status = glCheckNamedFramebufferStatus(fbo_, GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    spdlog::error("RenderTarget FBO incomplete after rebuild (0x{:X})", status);
  } else {
    spdlog::trace("RenderTarget({}) {}x{} created: colors={} depth={}", fbo_, width_, height_,
                  color_.size(), depth_ ? "yes" : "no");
  }
}

} // namespace blkhurst
