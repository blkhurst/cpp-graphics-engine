#pragma once
#include <blkhurst/geometry/geometry.hpp>
#include <blkhurst/loaders/texture_loader.hpp>
#include <blkhurst/materials/pbr_material.hpp>
#include <blkhurst/model/model_cpu.hpp>

#include <assimp/scene.h>
#include <unordered_map>

namespace blkhurst {

struct ModelProcessorDesc {
  bool genSmoothNormals = false;
  bool calcTangents = false;
  bool triangulate = true;
  bool optimize = true;
};

struct ModelProcessorContext {
  ModelProcessorDesc desc;
  std::string modelPath;
  std::unordered_map<std::string, std::shared_ptr<Texture>> textureCache;
};

struct ModelProcessor {
public:
  static ModelCPU load(const std::string& path, const ModelProcessorDesc& desc = {});

private:
  static NodeCPU processNode(const aiScene* scene, const aiNode* node,
                             ModelProcessorContext& context);
  static GeometryCPU processGeometry(const aiMesh& mesh);
  static std::shared_ptr<PbrMaterial> buildMaterial(const aiScene* scene, const aiMesh* sceneMesh,
                                                    const aiMaterial* material,
                                                    ModelProcessorContext& context);

  // ----- Helpers ------
  static unsigned int composeFlags(const ModelProcessorDesc& desc);
  static std::shared_ptr<Texture> loadTexture(const aiScene* scene, const aiString& texturePath,
                                              const TextureLoaderDesc& desc,
                                              ModelProcessorContext& context);
};

} // namespace blkhurst
