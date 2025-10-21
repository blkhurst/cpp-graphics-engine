#pragma once

#include <blkhurst/engine/root_state.hpp>
#include <blkhurst/util/identifiable.hpp>

#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <vector>

namespace blkhurst {

enum class NodeKind { Object, Mesh, Lines, Points, Light, Camera };

class Object3D : public Identifiable {
public:
  Object3D() = default;
  virtual ~Object3D() = default;

  Object3D(const Object3D&) = delete;
  Object3D& operator=(const Object3D&) = delete;
  Object3D(Object3D&&) = delete;
  Object3D& operator=(Object3D&&) = delete;

  Object3D* parent() const;
  const std::vector<std::unique_ptr<Object3D>>& children() const;

  template <class T> T* addChild(std::unique_ptr<T> child);
  template <class T, class... Args> T* addChild(Args&&... args);
  Object3D* findByName(const std::string& name, bool recursive);
  bool removeChild(Object3D* child);
  bool removeFromParent();

  virtual void onUpdate(const RootState& /*state*/);

  virtual NodeKind kind() const;
  bool visible() const;

  // Getters
  const glm::vec3& position() const;
  const glm::quat& rotation() const; // Quaternion, do not need euler angles currently.
  const glm::vec3& scale() const;

  const glm::mat4& matrix() const;
  const glm::mat4& worldMatrix() const;

  glm::vec3 worldPosition() const;
  glm::vec3 worldDirection() const;

  // Setters
  void setVisible(bool visible);

  void setPosition(const glm::vec3& position);
  void setRotation(const glm::quat& quat);
  void setScale(const glm::vec3& scale);
  void setWorldPosition(const glm::vec3& position);

  void rotateOnAxis(const glm::vec3& axis, float radians);
  void rotateOnWorldAxis(const glm::vec3& axis, float radians);
  void rotateX(float radians);
  void rotateY(float radians);
  void rotateZ(float radians);

  void translateOnAxis(const glm::vec3& axis, float distance);
  void translateOnWorldAxis(const glm::vec3& axis, float distance);
  void translateX(float distance);
  void translateY(float distance);
  void translateZ(float distance);

  void lookAt(const glm::vec3& target);

  void needsUpdate();
  void traverse(const std::function<void(Object3D&)>& func);
  std::unique_ptr<Object3D> clone(bool recursive = true) const;

protected:
  Object3D* addChild_(std::unique_ptr<Object3D> child);

private:
  Object3D* parent_ = nullptr;
  std::vector<std::unique_ptr<Object3D>> children_;

  bool visible_ = true;

  // TRS
  glm::vec3 position_{0.0F, 0.0F, 0.0F};
  glm::quat rotation_{1.0F, 0.0F, 0.0F, 0.0F};
  glm::vec3 scale_{1.0F, 1.0F, 1.0F};

  mutable glm::mat4 matrix_{1.0F};
  mutable glm::mat4 worldMatrix_{1.0F};

  mutable bool needsUpdate_ = true;
  void calculateMatrices() const;
};

// Template Definition
// Create, Move ownership, Return reference
template <class T, class... Args> T* Object3D::addChild(Args&&... args) {
  auto object = std::make_unique<T>(std::forward<Args>(args)...);
  auto* rawPtr = object.get();
  addChild_(std::move(object));
  return rawPtr;
}
// Move ownership, Return reference
template <class T> T* Object3D::addChild(std::unique_ptr<T> child) {
  static_assert(std::is_base_of_v<Object3D, T>, "T must derive from Object3D");
  return static_cast<T*>(addChild_(std::move(child)));
}

} // namespace blkhurst
