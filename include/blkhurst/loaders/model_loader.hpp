#pragma once
#include <blkhurst/geometry/geometry.hpp>
#include <blkhurst/loaders/texture_loader.hpp>
#include <blkhurst/materials/pbr_material.hpp>
#include <blkhurst/objects/group.hpp>

#include <assimp/scene.h>
#include <unordered_map>

namespace blkhurst {

struct ModelLoaderDesc {
  bool genSmoothNormals = false;
  bool calcTangents = false;
  bool triangulate = true;
  bool optimize = true;
};

struct ModelLoaderContext {
  ModelLoaderDesc desc;
  std::string modelPath;
  std::unordered_map<std::string, std::shared_ptr<Texture>> textureCache;
};

struct ModelLoader {
public:
  static std::unique_ptr<Group> load(const std::string& path, const ModelLoaderDesc& desc = {});

private:
  static std::unique_ptr<Group> processNode(const aiScene* scene, const aiNode* node,
                                            ModelLoaderContext& context);
  static std::shared_ptr<Geometry> buildGeometry(const aiMesh& mesh);
  static std::shared_ptr<PbrMaterial> buildMaterial(const aiScene* scene, const aiMesh* sceneMesh,
                                                    const aiMaterial* material,
                                                    ModelLoaderContext& context);

  // ----- Helpers ------
  static unsigned int composeFlags(const ModelLoaderDesc& desc);
  static std::shared_ptr<Texture> loadTexture(const aiScene* scene, const aiString& texturePath,
                                              const TextureLoaderDesc& desc,
                                              ModelLoaderContext& context);
  static std::unique_ptr<Group> makeFallbackModel();
};

} // namespace blkhurst
