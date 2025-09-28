#include "materials/equirect_material.hpp"
#include <blkhurst/cameras/ortho_camera.hpp>
#include <blkhurst/geometry/plane_geometry.hpp>
#include <blkhurst/objects/mesh.hpp>
#include <blkhurst/renderer/cube_render_target.hpp>
#include <blkhurst/renderer/renderer.hpp>

#include <glad/gl.h>
#include <spdlog/spdlog.h>

namespace blkhurst {

CubeRenderTarget::CubeRenderTarget(int size, const CubeRenderTargetDesc& desc)
    : size_(size),
      desc_(desc) {
  glCreateFramebuffers(1, &framebufferId_);
  rebuildAttachments_();
}

CubeRenderTarget::~CubeRenderTarget() {
  if (framebufferId_ != 0U) {
    glDeleteFramebuffers(1, &framebufferId_);
    spdlog::trace("CubeRenderTarget({}) destroyed", framebufferId_);
    framebufferId_ = 0U;
  }
}

std::shared_ptr<CubeRenderTarget> CubeRenderTarget::create(int size,
                                                           const CubeRenderTargetDesc& desc) {
  return std::make_shared<CubeRenderTarget>(size, desc);
}

unsigned CubeRenderTarget::id() const {
  return framebufferId_;
}

void CubeRenderTarget::setSize(int size) {
  size_ = std::max(1, size);
  rebuildAttachments_();
}

int CubeRenderTarget::size() const {
  return size_;
}

std::shared_ptr<CubeTexture> CubeRenderTarget::texture() const {
  return texture_;
}

std::shared_ptr<CubeTexture> CubeRenderTarget::depthTexture() const {
  return depthTexture_;
}

std::shared_ptr<CubeRenderTarget>
CubeRenderTarget::fromEquirect(Renderer& renderer, const std::shared_ptr<Texture>& equirect,
                               const CubeRenderTargetDesc& desc) {
  if (!equirect) {
    spdlog::error("CubeRenderTarget::fromEquirect called with null texture");
    return nullptr;
  }

  // Set CubeRenderTarget Size
  const int equirectWidth = equirect->width();
  const int equirectHeight = equirect->height();
  int faceSize = std::min(equirectWidth / 4, equirectHeight / 2);
  const int minFaceSize = 16;
  faceSize = std::max(faceSize, minFaceSize);

  auto cubeRenderTarget = CubeRenderTarget::create(faceSize, desc);

  // Fullscreen Quad
  auto camera = OrthoCamera::create(); // Unused by EquirectMaterial
  auto planeGeometry = PlaneGeometry::create({.width = 2.0F, .height = 2.0F});
  auto equirectMaterial = EquirectMaterial::create({.equirectTexture = equirect});
  auto equirectMesh = Mesh::create(planeGeometry, equirectMaterial);

  // Render into each face
  const int faceCount = 6;
  for (int face = 0; face < faceCount; ++face) {
    equirectMaterial->setFace(face);
    renderer.setRenderTarget(cubeRenderTarget.get(), face, /*mip*/ 0);
    renderer.render(*equirectMesh, *camera);
  }

  // Build mipmaps if needed
  if (desc.colorDesc.generateMipmaps) {
    if (auto cubeTexture = cubeRenderTarget->texture()) {
      cubeTexture->generateMipmaps();
    }
  }

  // TODO: Restore previous render target
  renderer.setRenderTarget(nullptr);

  spdlog::debug("CubeRenderTarget::fromEquirect created {}x{} (mips={}, depth={})", faceSize,
                faceSize, desc.colorDesc.generateMipmaps, desc.depthAttachment);

  return cubeRenderTarget;
}

void CubeRenderTarget::rebuildAttachments_() {
  if (framebufferId_ == 0U) {
    glCreateFramebuffers(1, &framebufferId_);
  }

  // Create color attachment
  // Attach a default face/mip; renderer can reattach per face/mip.
  TextureDesc colorDesc{
      .format = desc_.colorDesc.format,
      .minFilter = desc_.colorDesc.minFilter,
      .magFilter = desc_.colorDesc.magFilter,
      .wrapS = desc_.colorDesc.wrapS,
      .wrapT = desc_.colorDesc.wrapT,
      .generateMipmaps = desc_.colorDesc.generateMipmaps,
  };
  auto colorCube = CubeTexture::create(size_, colorDesc);
  glNamedFramebufferTextureLayer(framebufferId_, GL_COLOR_ATTACHMENT0, colorCube->id(), /*level*/ 0,
                                 /*layer(face)*/ 0);
  texture_ = std::move(colorCube);

  // Draw buffer
  glNamedFramebufferDrawBuffer(framebufferId_, GL_COLOR_ATTACHMENT0);

  // Create depth attachment
  depthTexture_.reset();
  if (desc_.depthAttachment) {
    TextureDesc depthDesc{
        .format = desc_.depthDesc.format,
        .minFilter = desc_.depthDesc.minFilter,
        .magFilter = desc_.depthDesc.magFilter,
        .wrapS = desc_.depthDesc.wrapS,
        .wrapT = desc_.depthDesc.wrapT,
        .generateMipmaps = desc_.depthDesc.generateMipmaps,
    };
    auto depthCube = CubeTexture::create(size_, depthDesc);

    GLenum attachment = GL_DEPTH_ATTACHMENT;
    if (Texture::isDepthStencilFormat(depthDesc.format)) {
      attachment = GL_DEPTH_STENCIL_ATTACHMENT;
    } else if (Texture::isDepthFormat(depthDesc.format)) {
      attachment = GL_DEPTH_ATTACHMENT;
    } else {
      spdlog::error("CubeRenderTarget depthDesc.format is not a depth format");
    }

    glNamedFramebufferTextureLayer(framebufferId_, attachment, depthCube->id(), /*level*/ 0,
                                   /*layer(face)*/ 0);
    depthTexture_ = std::move(depthCube);
  }

  // Check status
  const auto status = glCheckNamedFramebufferStatus(framebufferId_, GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    spdlog::error("CubeRenderTarget FBO incomplete after rebuild (0x{:X})", status);
  } else {
    spdlog::trace("CubeRenderTarget({}) {}x{} created: depth={}", framebufferId_, size_, size_,
                  depthTexture_ ? "yes" : "no");
  }
}

} // namespace blkhurst
