#pragma once
#include <blkhurst/cameras/camera.hpp>
#include <blkhurst/cameras/ortho_camera.hpp>
#include <blkhurst/geometry/plane_geometry.hpp>
#include <blkhurst/graphics/program.hpp>
#include <blkhurst/materials/material.hpp>
#include <blkhurst/objects/group.hpp>
#include <blkhurst/objects/mesh.hpp>

#include <algorithm>
#include <memory>

namespace blkhurst {

class ILoadingScreen : public Group {
public:
  ILoadingScreen() = default;
  virtual ~ILoadingScreen() = default;

  ILoadingScreen(const ILoadingScreen&) = delete;
  ILoadingScreen(ILoadingScreen&&) = delete;
  ILoadingScreen& operator=(const ILoadingScreen&) = delete;
  ILoadingScreen& operator=(ILoadingScreen&&) = delete;

  virtual void setOpacity(float alpha) = 0;
  virtual Camera* camera() const = 0;
};

class DefaultLoadingScreen : public ILoadingScreen {
public:
  DefaultLoadingScreen() {
    // Geometry
    auto geometry = PlaneGeometry::create({2.0F, 2.0F});

    // Material
    auto program =
        Program::createFromRegistry({.vert = "fullscreen_vert", .frag = "loading_screen_frag"});
    material_ = Material::create(program);
    material_->setBlend(true);
    material_->setDepthWrite(false);
    material_->setDepthTest(false);

    // Mesh
    auto mesh = Mesh::create(geometry, material_);
    mesh->setName("LoadingScreen");
    addChild(std::move(mesh));
  }

  static std::unique_ptr<DefaultLoadingScreen> create() {
    return std::make_unique<DefaultLoadingScreen>();
  }

  Camera* camera() const override {
    return camera_.get();
  }

  void setOpacity(float alpha) override {
    alpha = std::clamp(alpha, 0.0F, 1.0F);
    if (material_) {
      material_->setUniform("uOpacity", alpha);
    }
  }

private:
  std::shared_ptr<Material> material_;
  std::shared_ptr<OrthoCamera> camera_ = OrthoCamera::create();
};

} // namespace blkhurst
