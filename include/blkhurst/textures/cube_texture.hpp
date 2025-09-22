#pragma once

#include <blkhurst/textures/texture.hpp>

namespace blkhurst {

class CubeTexture : public Texture {
public:
  CubeTexture(int size, const TextureDesc& desc);
  ~CubeTexture() override = default;

  CubeTexture(const CubeTexture&) = delete;
  CubeTexture& operator=(const CubeTexture&) = delete;
  CubeTexture(CubeTexture&&) = delete;
  CubeTexture& operator=(CubeTexture&&) = delete;

  static std::shared_ptr<CubeTexture> create(int size, const TextureDesc& desc);

  // Override to support additional R wrap
  void setWrap(TextureWrap wrapS, TextureWrap wrapT, TextureWrap wrapR) const;

  // faceIndex [0..5] in +X,-X,+Y,-Y,+Z,-Z order
  void setFacePixels(int faceIndex, const void* pixels, int level = 0);
};

} // namespace blkhurst
