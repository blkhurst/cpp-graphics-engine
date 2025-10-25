#include <blkhurst/loaders/model_loader.hpp>
#include <blkhurst/objects/mesh.hpp>
#include <blkhurst/util/assets.hpp>

#include <assimp/GltfMaterial.h>
#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/texture.h>
#include <assimp/types.h>
#include <spdlog/spdlog.h>
#include <string>

namespace blkhurst {

std::unique_ptr<Group> ModelLoader::load(const std::string& path, const ModelProcessorDesc& desc) {
  // Load ModelCPU
  auto modelCPU = ModelProcessor::load(path, desc);
  if (!modelCPU.success) {
    spdlog::error("ModelLoader failed to load model '{}'", path);
    return std::make_unique<Group>();
  }

  // Convert ModelCPU to Group
  auto group = buildGroup(modelCPU.root);

  return group;
}

std::unique_ptr<Group> ModelLoader::buildGroup(const NodeCPU& node) {
  auto group = std::make_unique<Group>();
  if (!node.name.empty()) {
    group->setName(node.name);
  }

  // Transform
  group->setPosition(node.position);
  group->setRotation(node.rotation);
  group->setScale(node.scale);

  // Meshes on this node
  for (const MeshCPU& meshCPU : node.meshes) {
    auto geometry = buildGeometry(meshCPU.geometry);
    auto material = meshCPU.material;

    auto mesh = Mesh::create(geometry, material);
    if (!meshCPU.name.empty()) {
      mesh->setName(meshCPU.name);
    }
    group->addChild(std::move(mesh));
  }

  // Children
  for (const NodeCPU& child : node.children) {
    group->addChild(buildGroup(child));
  }

  return group;
}

std::shared_ptr<Geometry> ModelLoader::buildGeometry(const GeometryCPU& geometryData) {
  // Create Geometry
  auto geometry = Geometry::create();

  // Set Attributes
  geometry->setAttribute(Attrib::Position, geometryData.positions, 3);
  if (!geometryData.indices.empty()) {
    geometry->setIndex(geometryData.indices);
  }
  if (!geometryData.normals.empty()) {
    geometry->setAttribute(Attrib::Normal, geometryData.normals, 3);
  }
  if (!geometryData.uvs.empty()) {
    geometry->setAttribute(Attrib::Uv, geometryData.uvs, 2);
  }
  if (!geometryData.colors.empty()) {
    geometry->setAttribute(Attrib::Color, geometryData.colors, 3);
  }
  // if (!geomData.tangents.empty()) {
  //   geometry->setAttribute(Attrib::Tangent, geomData.tangents, 3);
  //   geometry->setAttribute(Attrib::Bitangent, geomData.bitangents, 3);
  //   // Bitangent can be reconstructed in shader
  // }
  // if (!geomData.uvs1.empty()) {
  //   geometry->setAttribute(Attrib::Uv1, geomData.uvs1, 2);
  // }

  return geometry;
}

} // namespace blkhurst
