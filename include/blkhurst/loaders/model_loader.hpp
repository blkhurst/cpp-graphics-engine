#pragma once
#include <blkhurst/geometry/geometry.hpp>
#include <blkhurst/loaders/texture_loader.hpp>
#include <blkhurst/materials/pbr_material.hpp>
#include <blkhurst/model/model_processor.hpp>
#include <blkhurst/objects/group.hpp>

#include <assimp/scene.h>

namespace blkhurst {

struct ModelLoaderDesc {
  bool genSmoothNormals = false;
  bool calcTangents = false;
  bool triangulate = true;
  bool optimize = true;
};

struct ModelLoader {
public:
  static std::unique_ptr<Group> load(const std::string& path, const ModelProcessorDesc& desc = {});
  static std::unique_ptr<Group> buildGroup(const NodeCPU& node); // Async Compatible

private:
  static std::shared_ptr<Geometry> buildGeometry(const GeometryCPU& geometryData);
};

} // namespace blkhurst
