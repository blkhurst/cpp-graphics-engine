#pragma once
#include <blkhurst/textures/cube_texture.hpp>
#include <blkhurst/textures/texture.hpp>

#include <memory>

namespace blkhurst {

class Renderer;

static constexpr int kDefaultIrradianceSize = 64;       // 16-64
static constexpr int kDefaultRadianceSize = 256;        // 128-512
static constexpr int kDefaultBrdfSize = 256;            // 256-512
static constexpr float kDefaultPrefilterLodBias = 2.0F; // 0-4
static constexpr int kDefaultGgxSamples = 1024;

struct PMREMResult {
  std::shared_ptr<Texture> brdfLUT;
  std::shared_ptr<CubeTexture> irradianceMap;
  std::shared_ptr<CubeTexture> prefilterMap;
};

struct PMREMDesc {
  // Texture Sizes
  int irradianceSize = kDefaultIrradianceSize;
  int radianceSize = kDefaultRadianceSize;
  int brdfSize = kDefaultBrdfSize;

  // Prefilter Options
  int ggxSamples = kDefaultGgxSamples;
  /// LOD bias to apply when prefiltering. ~[0..4] Increase to reduce bright artifacts.
  float prefilterLodBias = kDefaultPrefilterLodBias;
};

class PMREMGenerator {
public:
  PMREMGenerator(Renderer* renderer, const PMREMDesc& desc = {});
  PMREMResult fromEquirect(const std::shared_ptr<Texture>& equirect);
  PMREMResult fromCubemap(const std::shared_ptr<CubeTexture>& cubemap);

private:
  Renderer* renderer_;
  PMREMDesc desc_;
  std::shared_ptr<Texture> brdfLUT_;

  static std::shared_ptr<Texture> generateBRDFLUT(Renderer& renderer, int size);

  static std::shared_ptr<CubeTexture>
  integrateDiffuse(Renderer& renderer, const std::shared_ptr<CubeTexture>& src, int size);

  static std::shared_ptr<CubeTexture> prefilterSpecular(Renderer& renderer,
                                                        const std::shared_ptr<CubeTexture>& src,
                                                        int size, int ggxSamples, float lodBias);
};

} // namespace blkhurst
