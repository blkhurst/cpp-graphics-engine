#pragma once

#include <blkhurst/geometry/geometry.hpp>
#include <blkhurst/materials/material.hpp>
#include <blkhurst/objects/object3d.hpp>

#include <memory>

namespace blkhurst {

class Mesh : public Object3D {
public:
  Mesh(std::shared_ptr<Geometry> geom, std::shared_ptr<Material> mat);
  ~Mesh() override;

  Mesh(const Mesh&) = delete;
  Mesh& operator=(const Mesh&) = delete;
  Mesh(Mesh&&) = delete;
  Mesh& operator=(Mesh&&) = delete;

  static std::unique_ptr<Mesh> create(std::shared_ptr<Geometry> geom,
                                      std::shared_ptr<Material> mat) {
    return std::make_unique<Mesh>(std::move(geom), std::move(mat));
  }

  NodeKind kind() const override {
    return NodeKind::Mesh;
  }

  [[nodiscard]] std::shared_ptr<Geometry> geometry() const;
  [[nodiscard]] std::shared_ptr<Material> material() const;
  [[nodiscard]] int instanceCount() const;
  [[nodiscard]] bool wireframe() const;

  void setGeometry(std::shared_ptr<Geometry> geom);
  void setMaterial(std::shared_ptr<Material> mat);
  void setInstanceCount(int count);
  void setWireframe(bool enabled);

private:
  std::shared_ptr<Geometry> geometry_;
  std::shared_ptr<Material> material_;
  int instanceCount_ = 1;
  bool wireframe_ = false;
};

} // namespace blkhurst
