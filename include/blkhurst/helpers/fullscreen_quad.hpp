#pragma once
#include <blkhurst/cameras/ortho_camera.hpp>
#include <blkhurst/geometry/plane_geometry.hpp>
#include <blkhurst/materials/material.hpp>
#include <blkhurst/objects/mesh.hpp>
#include <blkhurst/renderer/renderer.hpp>

#include <memory>

namespace blkhurst {

struct FullscreenQuad {
  FullscreenQuad(const std::shared_ptr<Material>& material) {
    auto geometry = PlaneGeometry::create({2.0F, 2.0F});
    mesh_ = Mesh::create(geometry, material);
    camera_ = OrthoCamera::create();
  }

  void render(Renderer& renderer) {
    renderer.render(*mesh_, *camera_);
  }

private:
  std::unique_ptr<Mesh> mesh_;
  std::shared_ptr<OrthoCamera> camera_;
};

} // namespace blkhurst
