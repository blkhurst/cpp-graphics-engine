#include "ibl/brdf_lut_material.hpp"
#include "ibl/irradiance_material.hpp"
#include <blkhurst/cameras/ortho_camera.hpp>
#include <blkhurst/geometry/plane_geometry.hpp>
#include <blkhurst/ibl/pmrem_generator.hpp>
#include <blkhurst/renderer/renderer.hpp>

#include <spdlog/spdlog.h>

namespace blkhurst {

PMREMResult PMREMGenerator::fromEquirect(Renderer& renderer,
                                         const std::shared_ptr<Texture>& equirect,
                                         const PMREMDesc& desc) {
  if (!equirect) {
    spdlog::error("PMREMGenerator::fromEquirect null equirect");
    return {};
  }

  auto cubemap = CubeRenderTarget::fromEquirect(renderer, equirect)->texture();
  return fromCubemap(renderer, cubemap, desc);
}

PMREMResult PMREMGenerator::fromCubemap(Renderer& renderer,
                                        const std::shared_ptr<CubeTexture>& cubemap,
                                        const PMREMDesc& desc) {
  if (!cubemap) {
    spdlog::error("PMREMGenerator::fromCubemap null cubemap");
    return {};
  }

  PMREMResult out{};
  out.brdfLUT = generateBRDFLUT(renderer, desc.brdfSize);
  out.irradiance = integrateDiffuse(renderer, cubemap, desc.irradianceSize);
  // out.prefilteredSpecular =
  //     prefilterSpecular(renderer, cubemap, desc.radianceSize, desc.ggxSamples);
  return out;
}

std::shared_ptr<Texture> PMREMGenerator::generateBRDFLUT(Renderer& renderer, int size) {
  // RenderTarget
  RenderTargetDesc desc{};
  desc.colorAttachmentCount = 1;
  desc.colorDesc.format = TextureFormat::RGBA16F; // TODO: Use RG16F/RG32F instead
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

} // namespace blkhurst
