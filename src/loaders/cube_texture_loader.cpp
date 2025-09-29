#include <blkhurst/loaders/cube_texture_loader.hpp>
#include <blkhurst/loaders/texture_loader.hpp>
#include <blkhurst/util/assets.hpp>
#include <spdlog/spdlog.h>
#include <vector>

namespace blkhurst {

static constexpr int kOutputChannels = 4;

std::shared_ptr<CubeTexture>
CubeTextureLoader::load(const std::array<std::string, kCubeFaceCount>& paths,
                        const CubeTextureLoaderDesc& desc) {
  // Resolve asset paths
  std::vector<std::string> resolvedPaths{};
  resolvedPaths.reserve(kCubeFaceCount);
  for (auto path : paths) {
    auto found = assets::find(path);
    if (!found) {
      spdlog::error("CubeTextureLoader asset not found ({})", path);
      return makeFallback_();
    }
    resolvedPaths.push_back(*found);
  }

  // Read Face Pixels
  std::vector<LoadedPixels> faces{};
  faces.reserve(kCubeFaceCount);
  for (auto path : resolvedPaths) {
    LoadedPixels pixels = TextureLoader::readPixels(path, desc.flipY, kOutputChannels);
    if (!pixels.valid()) {
      spdlog::error("CubeTextureLoader failed to read ({})", path);
      for (auto& face : faces) {
        face.free();
      }
      return makeFallback_();
    }
    faces.push_back(pixels);
  }

  // Validate dimensions & HDR consistency
  if (!validateFaces_(faces)) {
    for (auto& face : faces) {
      face.free();
    }
    return makeFallback_();
  }

  const int size = faces[0].width;
  const bool isHdr = faces[0].isFloat;

  // Pick Format
  TextureFormat outputFormat = desc.srgb ? TextureFormat::SRGB8_ALPHA8 : TextureFormat::RGBA8;
  if (isHdr) {
    outputFormat = TextureFormat::RGBA32F; // Float
  }

  // Create CubeTexture
  TextureDesc textureDesc{};
  textureDesc.format = outputFormat;
  textureDesc.minFilter = desc.minFilter;
  textureDesc.magFilter = desc.magFilter;
  textureDesc.wrapS = desc.wrapS;
  textureDesc.wrapT = desc.wrapT;
  // textureDesc.wrapR = TextureWrap::ClampToEdge;
  textureDesc.generateMipmaps = desc.generateMipmaps;

  auto cubeTexture = CubeTexture::create(size, textureDesc);

  // Set CubeTexture Face Data
  for (int face = 0; face < kCubeFaceCount; ++face) {
    if (faces.at(face).isFloat) {
      cubeTexture->setFacePixels(face, faces.at(face).floats, /*level*/ 0);
    } else {
      cubeTexture->setFacePixels(face, faces.at(face).bytes, /*level*/ 0);
    }
  }

  // Free Pixels
  for (auto& face : faces) {
    face.free();
  }

  spdlog::debug("CubeTextureLoader loaded cubemap (size={} ch={} hdr={} srgb={})", size,
                kOutputChannels, isHdr, desc.srgb);
  return cubeTexture;
}

std::shared_ptr<CubeTexture> CubeTextureLoader::makeFallback_() {
  // 2×2 RGBA checker (magenta/black)
  const int kSize = 2;
  constexpr auto kCount = static_cast<std::size_t>(kSize) * kSize * 4;
  const std::array<unsigned char, kCount> pixels = {255, 0, 255, 255, 0,   0, 0,   255,
                                                    0,   0, 0,   255, 255, 0, 255, 255};

  TextureDesc desc{};
  desc.format = TextureFormat::RGBA8;
  desc.minFilter = TextureFilter::Nearest;
  desc.magFilter = TextureFilter::Nearest;
  desc.wrapS = TextureWrap::ClampToEdge;
  desc.wrapT = TextureWrap::ClampToEdge;
  // desc.wrapR = TextureWrap::ClampToEdge;
  desc.generateMipmaps = false;

  auto cube = CubeTexture::create(kSize, desc);
  for (std::size_t i = 0; i < kCubeFaceCount; ++i) {
    cube->setFacePixels(static_cast<int>(i), pixels.data(), 0);
  }

  spdlog::warn("CubeTextureLoader using fallback texture");
  return cube;
}

bool CubeTextureLoader::validateFaces_(const std::vector<LoadedPixels>& faces) {
  const int width0 = faces[0].width;
  const int height0 = faces[0].height;
  const bool isFloat0 = faces[0].isFloat;

  if (width0 <= 0 || height0 <= 0) {
    spdlog::error("CubeTextureLoader: invalid face size {}x{}", width0, height0);
    return false;
  }
  if (width0 != height0) {
    spdlog::warn("CubeTextureLoader: non-square faces ({}x{}). Cubemaps require square faces.",
                 width0, height0);
  }

  for (std::size_t i = 1; i < kCubeFaceCount; ++i) {
    if (faces.at(i).width != width0 || faces.at(i).height != height0) {
      spdlog::error("CubeTextureLoader: face {} size mismatch ({}x{} vs {}x{})", i,
                    faces.at(i).width, faces.at(i).height, width0, height0);
      return false;
    }
    if (faces.at(i).isFloat != isFloat0) {
      spdlog::error("CubeTextureLoader: face {} HDR/LDR mismatch", i);
      return false;
    }
    if (faces.at(i).channels != kOutputChannels) {
      spdlog::error("CubeTextureLoader: face {} channel mismatch (got {}, expected {})", i,
                    faces.at(i).channels, kOutputChannels);
      return false;
    }
  }
  return true;
}

} // namespace blkhurst
