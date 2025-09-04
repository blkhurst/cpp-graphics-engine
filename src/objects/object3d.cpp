#include <blkhurst/objects/object3d.hpp>
#include <random>
#include <spdlog/spdlog.h>

/**
 * Object3D
 * - Supports children, allowing nested transformations.
 * - Stores TRS (Translation, Rotation, Scale) independently to edit/interpolate
 *   without losing original state.
 * - worldMatrix = parent.worldMatrix * localModelMatrix, enabling grouping.
 * - needsUpdate propagates to children; world rebuilt lazily.
 */

namespace blkhurst {

Object3D::Object3D()
    : uuid_(make_uuid_()) {
}

Object3D* Object3D::add(std::unique_ptr<Object3D> child) {
  child->parent_ = this;
  child->needsUpdate();
  spdlog::trace("Object3D({}) add child Object3D({})", uuid_, child->uuid_);
  children_.push_back(std::move(child));
  return children_.back().get();
}

void Object3D::onUpdate(const RootState& /*state*/) {
  // Default
}

Object3D* Object3D::parent() const {
  return parent_;
}

const std::vector<std::unique_ptr<Object3D>>& Object3D::children() const {
  return children_;
}

std::uint64_t Object3D::uuid() const {
  return uuid_;
}
const std::string& Object3D::name() const {
  return name_;
}

NodeKind Object3D::kind() const {
  return NodeKind::Object;
}

bool Object3D::visible() const {
  return visible_;
}

const glm::vec3& Object3D::position() const {
  return position_;
}

const glm::quat& Object3D::rotation() const {
  return rotation_;
}

const glm::vec3& Object3D::scale() const {
  return scale_;
}

void Object3D::setName(std::string n) {
  spdlog::trace("Object3D({}) setName '{}'", uuid_, n);
  name_ = std::move(n);
}

void Object3D::setVisible(bool visible) {
  visible_ = visible;
}

void Object3D::setPosition(const glm::vec3& position) {
  position_ = position;
  needsUpdate();
}

void Object3D::setScale(const glm::vec3& scale) {
  scale_ = scale;
  needsUpdate();
}

void Object3D::setRotation(const glm::quat& quat) {
  rotation_ = glm::normalize(quat);
  needsUpdate();
}

void Object3D::rotateX(float radians) {
  const glm::quat delta = glm::angleAxis(radians, glm::vec3(1, 0, 0));
  rotation_ = glm::normalize(delta * rotation_);
  needsUpdate();
}

void Object3D::rotateY(float radians) {
  const glm::quat delta = glm::angleAxis(radians, glm::vec3(0, 1, 0));
  rotation_ = glm::normalize(delta * rotation_);
  needsUpdate();
}

void Object3D::rotateZ(float radians) {
  const glm::quat delta = glm::angleAxis(radians, glm::vec3(0, 0, 1));
  rotation_ = glm::normalize(delta * rotation_);
  needsUpdate();
}

void Object3D::translateX(float distance) {
  position_ += rotation_ * glm::vec3(distance, 0.0F, 0.0F);
  needsUpdate();
}

void Object3D::translateY(float distance) {
  position_ += rotation_ * glm::vec3(0.0F, distance, 0.0F);
  needsUpdate();
}

void Object3D::translateZ(float distance) {
  position_ += rotation_ * glm::vec3(0.0F, 0.0F, distance);
  needsUpdate();
}

// Mark this node and children as requiring rebuild on next access.
void Object3D::needsUpdate() {
  needsUpdate_ = true;
  for (auto& child : children_) {
    child->needsUpdate();
  }
}

// NOLINTBEGIN(readability-identifier-length)
const glm::mat4& Object3D::worldMatrix() const {
  if (needsUpdate_) {
    glm::mat4 T = glm::translate(glm::mat4(1.0F), position_);
    glm::mat4 R = glm::mat4_cast(rotation_);
    glm::mat4 S = glm::scale(glm::mat4(1.0F), scale_);
    glm::mat4 local = T * R * S;

    worldMatrix_ = (parent_ != nullptr) ? parent_->worldMatrix() * local : local;
    needsUpdate_ = false;
  }
  return worldMatrix_;
}
// NOLINTEND(readability-identifier-length)

void Object3D::traverse(const std::function<void(Object3D&)>& func) {
  func(*this);
  for (auto& child : children_) {
    child->traverse(func);
  }
}

std::uint64_t Object3D::make_uuid_() {
  static std::mt19937_64 rng{std::random_device{}()};
  static std::uniform_int_distribution<std::uint64_t> dist;
  return dist(rng);
}

} // namespace blkhurst
