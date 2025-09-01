#pragma once

#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <string>
#include <vector>

namespace blkhurst {

class RootState; // forward-declare

enum class NodeKind { Object, Mesh, Lines, Points, Light, Camera };

class Object3D {
public:
  Object3D();
  virtual ~Object3D() = default;

  Object3D(const Object3D&) = delete;
  Object3D& operator=(const Object3D&) = delete;
  Object3D(Object3D&&) = delete;
  Object3D& operator=(Object3D&&) = delete;

  Object3D* parent() const;
  const std::vector<std::unique_ptr<Object3D>>& children() const;

  Object3D* add(std::unique_ptr<Object3D> child);
  template <class T, class... Args> T* emplace(Args&&... args);

  virtual void onUpdate(const RootState& /*state*/);

  std::uint64_t uuid() const;
  const std::string& name() const;
  virtual NodeKind kind() const;
  bool visible() const;

  const glm::vec3& position() const;
  const glm::quat& rotation() const;
  const glm::vec3& scale() const;

  void setName(std::string n);
  void setVisible(bool visible);
  void setPosition(const glm::vec3& position);
  void setScale(const glm::vec3& scale);
  void setRotation(const glm::quat& quat);

  void rotateX(float radians);
  void rotateY(float radians);
  void rotateZ(float radians);
  void translateX(float distance);
  void translateY(float distance);
  void translateZ(float distance);

  void needsUpdate();
  const glm::mat4& worldMatrix() const;

  void traverse(const std::function<void(Object3D&)>& func);

private:
  Object3D* parent_ = nullptr;
  std::vector<std::unique_ptr<Object3D>> children_;

  std::uint64_t uuid_{0};
  std::string name_;
  bool visible_ = true;

  // TRS
  glm::vec3 position_{0.0F, 0.0F, 0.0F};
  glm::quat rotation_{1.0F, 0.0F, 0.0F, 0.0F};
  glm::vec3 scale_{1.0F, 1.0F, 1.0F};

  mutable glm::mat4 worldMatrix_{1.0F};
  mutable bool needsUpdate_ = true;

  static std::uint64_t make_uuid_();
};

// Template Definition
// Create, Move ownership, Return reference
template <class T, class... Args> T* Object3D::emplace(Args&&... args) {
  auto object = std::make_unique<T>(std::forward<Args>(args)...);
  T* raw = object.get();
  add(std::move(object));
  return raw;
}

} // namespace blkhurst
