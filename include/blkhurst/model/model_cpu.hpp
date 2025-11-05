#pragma once
#include <blkhurst/loaders/texture_loader.hpp>
#include <blkhurst/materials/pbr_material.hpp>

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

namespace blkhurst {

struct GeometryCPU {
  std::vector<float> positions;
  std::vector<unsigned> indices;
  std::vector<float> normals;
  std::vector<float> uvs;
  std::vector<float> colors;
};

struct MeshCPU {
  std::string name;
  GeometryCPU geometry;
  std::shared_ptr<PbrMaterial> material; // No GL calls until first use
};

struct NodeCPU {
  std::string name;

  glm::vec3 position{0.0F};
  glm::quat rotation{1, 0, 0, 0};
  glm::vec3 scale{1.0F};

  std::vector<MeshCPU> meshes;
  std::vector<NodeCPU> children;
};

struct ModelCPU {
  NodeCPU root;
  std::string sourcePath;

  bool success = true;
  unsigned meshCount = 0;
  unsigned materialCount = 0;
  unsigned textureCount = 0;
};

} // namespace blkhurst
