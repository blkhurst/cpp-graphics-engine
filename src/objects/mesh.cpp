#include <algorithm>
#include <blkhurst/objects/mesh.hpp>
#include <spdlog/spdlog.h>

namespace blkhurst {

Mesh::Mesh(std::shared_ptr<Geometry> geometry, std::shared_ptr<Material> material)
    : geometry_(std::move(geometry)),
      material_(std::move(material)) {
  if (!geometry_ || !material_) {
    spdlog::error("Mesh requires non-null Geometry and Material.");
  }
  spdlog::trace("Mesh({}) constructed", uuid());
}

Mesh::~Mesh() {
  spdlog::trace("Mesh({}) destroyed", uuid());
}

std::shared_ptr<Geometry> Mesh::geometry() const {
  return geometry_;
}

std::shared_ptr<Material> Mesh::material() const {
  return material_;
}

int Mesh::instanceCount() const {
  return instanceCount_;
}

bool Mesh::wireframe() const {
  return wireframe_;
}

void Mesh::setGeometry(std::shared_ptr<Geometry> geometry) {
  geometry_ = std::move(geometry);
  spdlog::trace("Mesh({}) setGeometry {}", uuid(), geometry_ ? "OK" : "null");
}

void Mesh::setMaterial(std::shared_ptr<Material> material) {
  material_ = std::move(material);
  spdlog::trace("Mesh({}) setMaterial {}", uuid(), material_ ? "OK" : "null");
}

void Mesh::setInstanceCount(int count) {
  instanceCount_ = std::max(1, count);
  spdlog::trace("Mesh({}) setInstanceCount {}", uuid(), instanceCount_);
}

void Mesh::setWireframe(bool enabled) {
  wireframe_ = enabled;
  spdlog::trace("Mesh({}) setWireframe {}", uuid(), wireframe_);
}

} // namespace blkhurst
