#pragma once

#include <blkhurst/geometry/geometry.hpp>
#include <blkhurst/materials/material.hpp>
#include <blkhurst/objects/object3d.hpp>

#include <glm/glm.hpp>
#include <memory>
#include <span>
#include <vector>

namespace blkhurst {

class Mesh : public Object3D {
public:
  Mesh(std::shared_ptr<Geometry> geometry, std::shared_ptr<Material> material);
  ~Mesh() override;

  Mesh(const Mesh&) = delete;
  Mesh& operator=(const Mesh&) = delete;
  Mesh(Mesh&&) = delete;
  Mesh& operator=(Mesh&&) = delete;

  static std::unique_ptr<Mesh> create(std::shared_ptr<Geometry> geometry,
                                      std::shared_ptr<Material> material) {
    return std::make_unique<Mesh>(std::move(geometry), std::move(material));
  }

  [[nodiscard]] std::shared_ptr<Geometry> geometry() const;
  [[nodiscard]] std::shared_ptr<Material> material() const;
  [[nodiscard]] int instanceCount() const;
  [[nodiscard]] bool hasInstanceColors() const;
  [[nodiscard]] std::span<const glm::mat4> instanceMatrices() const;
  [[nodiscard]] std::span<const glm::vec4> instanceColors() const;
  [[nodiscard]] bool wireframe() const;

  void setGeometry(std::shared_ptr<Geometry> geometry);
  void setMaterial(std::shared_ptr<Material> material);
  void setInstanceMatrices(std::span<const glm::mat4> matrices);
  void setInstanceColors(std::span<const glm::vec4> colors);
  void clearInstanceColors();
  void setWireframe(bool enabled);

  std::unique_ptr<Mesh> clone(bool recursive = true) const;

private:
  std::shared_ptr<Geometry> geometry_;
  std::shared_ptr<Material> material_;
  int instanceCount_ = 1;
  std::vector<glm::mat4> instanceMatrices_;
  std::vector<glm::vec4> instanceColors_;
  bool wireframe_ = false;
};

} // namespace blkhurst
