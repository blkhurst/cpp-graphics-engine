#include "ibl/brdf_lut_material.hpp"
#include "ibl/irradiance_material.hpp"
#include "ibl/prefilter_ggx_material.hpp"
#include <blkhurst/cameras/ortho_camera.hpp>
#include <blkhurst/geometry/plane_geometry.hpp>
#include <blkhurst/ibl/pmrem_generator.hpp>
#include <blkhurst/renderer/renderer.hpp>

#include <spdlog/spdlog.h>

namespace blkhurst {

PMREMGenerator::PMREMGenerator(Renderer* renderer, const PMREMDesc& desc)
    : renderer_(renderer),
      desc_(desc) {
}

PMREMResult PMREMGenerator::fromEquirect(const std::shared_ptr<Texture>& equirect) {
  if (!equirect || (renderer_ == nullptr)) {
    spdlog::error("PMREMGenerator::fromEquirect null equirect or renderer");
    return {};
  }

  auto cubemap = CubeRenderTarget::fromEquirect(*renderer_, equirect)->texture();
  return fromCubemap(cubemap);
}

PMREMResult PMREMGenerator::fromCubemap(const std::shared_ptr<CubeTexture>& cubemap) {
  if (!cubemap || (renderer_ == nullptr)) {
    spdlog::error("PMREMGenerator::fromCubemap null cubemap or renderer");
    return {};
  }

  if (!brdfLUT_) { // Cache BRDF LUT
    brdfLUT_ = generateBRDFLUT(*renderer_, desc_.brdfSize);
  }

  PMREMResult out{};
  out.brdfLUT = brdfLUT_;
  out.irradianceMap = integrateDiffuse(*renderer_, cubemap, desc_.irradianceSize);
  out.prefilterMap = prefilterSpecular(*renderer_, cubemap, desc_.radianceSize, desc_.ggxSamples,
                                       desc_.prefilterLodBias);
  return out;
}

std::shared_ptr<Texture> PMREMGenerator::generateBRDFLUT(Renderer& renderer, int size) {
  // RenderTarget
  RenderTargetDesc desc{};
  desc.colorAttachmentCount = 1;
  desc.colorDesc.format = TextureFormat::RG32F; // Requires only RG channels
  desc.colorDesc.minFilter = TextureFilter::Linear;
  desc.colorDesc.magFilter = TextureFilter::Linear;
  desc.colorDesc.wrapS = TextureWrap::ClampToEdge;
  desc.colorDesc.wrapT = TextureWrap::ClampToEdge;
  desc.colorDesc.generateMipmaps = false;
  desc.depthAttachment = false;
  auto renderTarget = RenderTarget::create(size, size, desc);

  // Fullscreen Quad
  auto camera = OrthoCamera::create();
  auto planeGeometry = PlaneGeometry::create({2.0F, 2.0F});
  auto brdfMaterial = BrdfLUTMaterial::create();
  auto fullscreen = Mesh::create(planeGeometry, brdfMaterial);

  // Render
  renderer.setRenderTarget(renderTarget.get());
  renderer.render(*fullscreen, *camera);
  renderer.setRenderTarget(nullptr);
  return renderTarget->texture();
}

std::shared_ptr<CubeTexture>
PMREMGenerator::integrateDiffuse(Renderer& renderer, const std::shared_ptr<CubeTexture>& src,
                                 int size) {
  // CubeRenderTarget
  CubeRenderTargetDesc desc{};
  desc.colorDesc.format = TextureFormat::RGBA16F; // TODO: Float16/Float32 Toggle
  desc.colorDesc.minFilter = TextureFilter::Linear;
  desc.colorDesc.magFilter = TextureFilter::Linear;
  desc.colorDesc.wrapS = TextureWrap::ClampToEdge;
  desc.colorDesc.wrapT = TextureWrap::ClampToEdge;
  desc.colorDesc.generateMipmaps = false;
  desc.depthAttachment = false;
  auto cubeRenderTarget = CubeRenderTarget::create(size, desc);

  // Fullscreen Quad
  auto camera = OrthoCamera::create();
  auto planeGeometry = PlaneGeometry::create({2.0F, 2.0F});
  auto irradianceMaterial = IrradianceMaterial::create(src, src->width());
  auto fullscreen = Mesh::create(planeGeometry, irradianceMaterial);

  // Render Each Face
  const int faceCount = 6;
  for (int face = 0; face < faceCount; ++face) {
    irradianceMaterial->setFace(face);
    renderer.setRenderTarget(cubeRenderTarget.get(), face, /*mip*/ 0);
    renderer.render(*fullscreen, *camera);
  }

  // Return Texture
  renderer.setRenderTarget(nullptr);
  return cubeRenderTarget->texture();
}

std::shared_ptr<CubeTexture>
PMREMGenerator::prefilterSpecular(Renderer& renderer, const std::shared_ptr<CubeTexture>& src,
                                  int size, int ggxSamples, float lodBias) {
  // CubeRenderTarget With Mipmaps (We then render each mip manually)
  CubeRenderTargetDesc desc{};
  desc.colorDesc.format = TextureFormat::RGBA16F; // TODO: Float16/Float32 Toggle
  desc.colorDesc.minFilter = TextureFilter::LinearMipmapLinear;
  desc.colorDesc.magFilter = TextureFilter::Linear;
  desc.colorDesc.wrapS = TextureWrap::ClampToEdge;
  desc.colorDesc.wrapT = TextureWrap::ClampToEdge;
  desc.colorDesc.generateMipmaps = true;
  desc.depthAttachment = false;
  auto cubeRenderTarget = CubeRenderTarget::create(size, desc);

  // Fullscreen Quad
  auto camera = OrthoCamera::create();
  auto planeGeometry = PlaneGeometry::create({2.0F, 2.0F});
  auto prefilterMaterial = PrefilterGGXMaterial::create(src, {.lodBias = lodBias});
  auto fullscreen = Mesh::create(planeGeometry, prefilterMaterial);

  // Calculate mip levels.
  // ThreeJS uses a 2D atlas and generates several mips >= 16x16 (MIN_LOD=4).
  // Since we are using mipmaps directly, we ignore mips < 16x16.
  // This prevents morphing of IBL reflections at high roughness.
  const int MIN_LOD = 4;
  const int mipCount = std::max(1, Texture::calcMipLevels(size, size) - MIN_LOD);
  const int maxMip = mipCount - 1; // E.g. 256x256 has 9 mips [0..8]

  // Render Each Mip Level
  for (int mip = 0; mip <= maxMip; ++mip) {
    float roughness = (float)mip / (float)maxMip;
    prefilterMaterial->setRoughness(roughness);
    prefilterMaterial->setGgxSamples(ggxSamples);

    // Render Each Face
    const int faceCount = 6;
    for (int face = 0; face < faceCount; ++face) {
      prefilterMaterial->setFace(face);
      renderer.setRenderTarget(cubeRenderTarget.get(), face, mip);
      renderer.render(*fullscreen, *camera);
    }
  }

  // Set mip range to avoid sampling incomplete mips
  cubeRenderTarget->texture()->setMipmapRange(0, maxMip);

  // Return Texture
  renderer.setRenderTarget(nullptr);
  return cubeRenderTarget->texture();
}

} // namespace blkhurst
