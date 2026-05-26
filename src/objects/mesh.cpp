#include <algorithm>
#include <blkhurst/objects/mesh.hpp>
#include <blkhurst/objects/object3d.hpp>
#include <spdlog/spdlog.h>

namespace blkhurst {

Mesh::Mesh(std::shared_ptr<Geometry> geometry, std::shared_ptr<Material> material)
    : Object3D(NodeType::Mesh),
      geometry_(std::move(geometry)),
      material_(std::move(material)) {
  if (!geometry_ || !material_) {
    spdlog::error("Mesh requires non-null Geometry and Material.");
  }
  spdlog::trace("Mesh({}) constructed", uuidString());
}

Mesh::~Mesh() {
  spdlog::trace("Mesh({}) destroyed", uuidString());
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

bool Mesh::hasInstanceColors() const {
  return !instanceColors_.empty();
}

std::span<const glm::mat4> Mesh::instanceMatrices() const {
  return instanceMatrices_;
}

std::span<const glm::vec4> Mesh::instanceColors() const {
  return instanceColors_;
}

bool Mesh::wireframe() const {
  return wireframe_;
}

void Mesh::setGeometry(std::shared_ptr<Geometry> geometry) {
  geometry_ = std::move(geometry);
  spdlog::trace("Mesh({}) setGeometry {}", uuidString(), geometry_ ? "OK" : "null");
}

void Mesh::setMaterial(std::shared_ptr<Material> material) {
  material_ = std::move(material);
  spdlog::trace("Mesh({}) setMaterial {}", uuidString(), material_ ? "OK" : "null");
}

void Mesh::setInstanceMatrices(std::span<const glm::mat4> matrices) {
  instanceMatrices_.assign(matrices.begin(), matrices.end());
  instanceCount_ = std::max(1, static_cast<int>(instanceMatrices_.size()));
  if (!instanceColors_.empty()) {
    instanceColors_.resize(instanceCount_, glm::vec4(1.0F));
  }
  spdlog::trace("Mesh({}) setInstanceMatrices {}", uuidString(), instanceCount_);
}

void Mesh::setInstanceColors(std::span<const glm::vec4> colors) {
  instanceColors_.assign(colors.begin(), colors.end());
  instanceCount_ = std::max(instanceCount_, static_cast<int>(instanceColors_.size()));
  instanceMatrices_.resize(instanceCount_, glm::mat4(1.0F));
  instanceColors_.resize(instanceCount_, glm::vec4(1.0F));
  spdlog::trace("Mesh({}) setInstanceColors {}", uuidString(), instanceColors_.size());
}

void Mesh::clearInstanceColors() {
  instanceColors_.clear();
  spdlog::trace("Mesh({}) clearInstanceColors", uuidString());
}

void Mesh::setWireframe(bool enabled) {
  wireframe_ = enabled;
  spdlog::trace("Mesh({}) setWireframe {}", uuidString(), wireframe_);
}

// Shallow copy of Geometry and Material
std::unique_ptr<Mesh> Mesh::clone(bool recursive) const {
  auto copy = std::make_unique<Mesh>(geometry_, material_);
  // Copy Object3D state
  copy->setName(name());
  copy->setVisible(visible());
  copy->setPosition(position());
  copy->setRotation(rotation());
  copy->setScale(scale());
  // Copy Mesh state; sets needsUpdate_ internally
  copy->setInstanceMatrices(instanceMatrices_);
  if (!instanceColors_.empty()) {
    copy->setInstanceColors(instanceColors_);
  }
  copy->setWireframe(wireframe_);

  if (recursive) {
    for (const auto& child : children()) {
      copy->addChild_(child->clone(true));
    }
  }
  return copy;
}
} // namespace blkhurst
