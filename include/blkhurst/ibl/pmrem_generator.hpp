#pragma once
#include <blkhurst/textures/cube_texture.hpp>
#include <blkhurst/textures/texture.hpp>

#include <memory>

namespace blkhurst {

class Renderer;

struct PMREMResult {
  std::shared_ptr<Texture> brdfLUT;
  std::shared_ptr<CubeTexture> irradiance;
  std::shared_ptr<CubeTexture> prefilteredSpecular;
};

struct PMREMDesc {
  int irradianceSize = kDefaultIrradianceSize; // 16–64 is typical
  int radianceSize = kDefaultRadianceSize;     // power of two
  int brdfSize = kDefaultBrdfSize;             // 256 or 512
  int ggxSamples = kDefaultGgxSamples;         // importance samples per pixel for GGX prefilter

private:
  static constexpr int kDefaultIrradianceSize = 32;
  static constexpr int kDefaultRadianceSize = 256;
  static constexpr int kDefaultBrdfSize = 256;
  static constexpr int kDefaultGgxSamples = 1024;
};

struct PMREMGenerator {
  static PMREMResult fromEquirect(Renderer& renderer, const std::shared_ptr<Texture>& equirect,
                                  const PMREMDesc& desc = {});

  static PMREMResult fromCubemap(Renderer& renderer, const std::shared_ptr<CubeTexture>& cubemap,
                                 const PMREMDesc& desc = {});

  static std::shared_ptr<Texture> generateBRDFLUT(Renderer& renderer, int size);

  static std::shared_ptr<CubeTexture>
  integrateDiffuse(Renderer& renderer, const std::shared_ptr<CubeTexture>& src, int size);

  static std::shared_ptr<CubeTexture> prefilterSpecular(Renderer& renderer,
                                                        const std::shared_ptr<CubeTexture>& src,
                                                        int size, int sampleCount);
};

} // namespace blkhurst
