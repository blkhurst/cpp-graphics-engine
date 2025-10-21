#include <blkhurst/objects/object3d.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/orthonormalize.hpp>
#include <spdlog/spdlog.h>

/**
 * Object3D
 * - Supports children, allowing nested transformations.
 * - Stores TRS (Translation, Rotation, Scale) independently to edit/interpolate
 *   without losing original state.
 * - worldMatrix = parent.worldMatrix * localModelMatrix, enabling grouping.
 * - needsUpdate propagates to children; world rebuilt lazily.
 * - `lookAt` orients +Z towards target, -Z towards target for Cameras & Lights.
 */

namespace {
glm::quat extractRotationQ(const glm::mat4& matrix) {
  return glm::normalize(glm::quat_cast(glm::orthonormalize(glm::mat3(matrix))));
}
} // namespace

namespace blkhurst {

Object3D::Object3D(NodeType nodeType)
    : type_(nodeType) {
}

void Object3D::onUpdate(const RootState& /*state*/) {
}

Object3D* Object3D::parent() const {
  return parent_;
}

const std::vector<std::unique_ptr<Object3D>>& Object3D::children() const {
  return children_;
}

Object3D* Object3D::findByName(const std::string& name, bool recursive) {
  for (auto& child : children_) {
    if (child->name() == name) {
      return child.get();
    }
    if (recursive) {
      if (auto* rChild = child->findByName(name, true)) {
        return rChild;
      }
    }
  }
  return nullptr;
}

bool Object3D::removeChild(Object3D* child) {
  auto found = std::find_if(children_.begin(), children_.end(),
                            [&](auto& node) { return node.get() == child; });
  if (found == children_.end()) {
    return false;
  }
  (*found)->parent_ = nullptr;
  children_.erase(found);
  return true;
}

bool Object3D::removeFromParent() {
  return (parent_ != nullptr) ? parent_->removeChild(this) : false;
}

NodeType Object3D::type() const {
  return type_;
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

const glm::mat4& Object3D::matrix() const {
  calculateMatrices();
  return matrix_;
}

const glm::mat4& Object3D::worldMatrix() const {
  calculateMatrices();
  return worldMatrix_;
}

glm::vec3 Object3D::worldPosition() const {
  const glm::mat4& world = worldMatrix();
  auto position = glm::vec3(world[3]); // Translation column
  return position;
}

glm::vec3 Object3D::worldDirection() const {
  glm::mat3 worldRotation = glm::orthonormalize(glm::mat3(worldMatrix()));
  return glm::normalize(worldRotation * glm::vec3(0, 0, 1)); // +Z
}

void Object3D::setVisible(bool visible) {
  visible_ = visible;
}

void Object3D::setPosition(const glm::vec3& position) {
  position_ = position;
  needsUpdate();
}

void Object3D::setRotation(const glm::quat& quat) {
  rotation_ = glm::normalize(quat);
  needsUpdate();
}

void Object3D::setScale(const glm::vec3& scale) {
  scale_ = scale;
  needsUpdate();
}

void Object3D::setWorldPosition(const glm::vec3& position) {
  if (parent_ != nullptr) {
    const glm::mat4 parentWorld = parent_->worldMatrix();
    const glm::mat4 invParent = glm::inverse(parentWorld);
    const glm::vec4 local = invParent * glm::vec4(position, 1.0F);
    setPosition(glm::vec3(local));
  } else {
    setPosition(position);
  }
}

void Object3D::rotateOnAxis(const glm::vec3& axis, float radians) {
  // (Local-space rotation)
  // Start with existing rotation_, then apply delta (post-multiply)
  const glm::vec3 normalisedAxis = glm::normalize(axis);
  const glm::quat delta = glm::angleAxis(radians, normalisedAxis);
  rotation_ = glm::normalize(rotation_ * delta);
  needsUpdate();
}

void Object3D::rotateOnWorldAxis(const glm::vec3& axisW, float radians) {
  const glm::quat deltaQuat = glm::angleAxis(radians, glm::normalize(axisW));

  if (parent_ != nullptr) {
    glm::quat parentQ = extractRotationQ(parent_->worldMatrix());
    glm::quat localDelta = glm::inverse(parentQ) * deltaQuat * parentQ;
    rotation_ = glm::normalize(localDelta * rotation_);
  } else {
    rotation_ = glm::normalize(deltaQuat * rotation_);
  }
  needsUpdate();
}

void Object3D::rotateX(float radians) {
  rotateOnAxis({1.0, 0.0, 0.0}, radians);
}

void Object3D::rotateY(float radians) {
  rotateOnAxis({0.0, 1.0, 0.0}, radians);
}

void Object3D::rotateZ(float radians) {
  rotateOnAxis({0.0, 0.0, 1.0}, radians);
}

void Object3D::translateOnAxis(const glm::vec3& axis, float distance) {
  glm::vec3 localDelta = glm::normalize(axis) * distance;
  position_ += rotation_ * localDelta;
  needsUpdate();
}

void Object3D::translateOnWorldAxis(const glm::vec3& axis, float distance) {
  const glm::vec3 worldAxis = glm::normalize(axis);
  const glm::vec3 newWorld = worldPosition() + worldAxis * distance;
  setWorldPosition(newWorld);
}

void Object3D::translateX(float distance) {
  translateOnAxis({1.0, 0.0, 0.0}, distance);
}

void Object3D::translateY(float distance) {
  translateOnAxis({0.0, 1.0, 0.0}, distance);
}

void Object3D::translateZ(float distance) {
  translateOnAxis({0.0, 0.0, 1.0}, distance);
}

// Orient +Z towards for Objects, -Z for Cameras & Lights
void Object3D::lookAt(const glm::vec3& targetWorld) {
  glm::vec3 upVec = {0, 1, 0};
  glm::vec3 worldPos = worldPosition();

  bool isLightOrCamera = (type() == NodeType::Camera || type() == NodeType::Light);

  // View Matrix - Eye, Center, Up
  glm::mat4 viewMat = isLightOrCamera ? glm::lookAt(worldPos, targetWorld, upVec)
                                      : glm::lookAt(targetWorld, worldPos, upVec);

  // Convert viewMat to world
  glm::mat4 worldMat = glm::inverse(viewMat);
  glm::quat worldQ = glm::quat_cast(glm::mat3(worldMat));

  if (parent_ != nullptr) {
    // Convert world rotation to local
    auto parentQ = extractRotationQ(parent_->worldMatrix());
    rotation_ = glm::normalize(glm::inverse(parentQ) * worldQ);
  } else {
    rotation_ = glm::normalize(worldQ);
  }

  needsUpdate();
}

// Mark this node and children as requiring rebuild on next access.
void Object3D::needsUpdate() {
  needsUpdate_ = true;
  for (auto& child : children_) {
    child->needsUpdate();
  }
}

void Object3D::traverse(const std::function<void(Object3D&)>& func) {
  func(*this);
  for (auto& child : children_) {
    child->traverse(func);
  }
}

// NOLINTBEGIN(readability-identifier-length)
void Object3D::calculateMatrices() const {
  if (needsUpdate_) {
    glm::mat4 T = glm::translate(glm::mat4(1.0F), position_);
    glm::mat4 R = glm::mat4_cast(rotation_);
    glm::mat4 S = glm::scale(glm::mat4(1.0F), scale_);
    glm::mat4 local = T * R * S;

    matrix_ = local;
    worldMatrix_ = (parent_ != nullptr) ? parent_->worldMatrix() * local : local;
    needsUpdate_ = false;
  }
}
// NOLINTEND(readability-identifier-length)

// Deep copy
std::unique_ptr<Object3D> Object3D::clone(bool recursive) const {
  auto copy = std::make_unique<Object3D>(type_);
  // copy->name_ = name_;
  copy->setName(name());
  copy->visible_ = visible_;
  copy->position_ = position_;
  copy->rotation_ = rotation_;
  copy->scale_ = scale_;
  copy->needsUpdate_ = true;

  if (recursive) {
    for (const auto& child : children_) {
      copy->addChild_(child->clone(true));
    }
  }
  return copy;
}

Object3D* Object3D::addChild_(std::unique_ptr<Object3D> child) {
  child->parent_ = this;
  child->needsUpdate();
  spdlog::trace("Object3D({}) add child Object3D({})", uuidString(), child->uuidString());
  children_.push_back(std::move(child));
  return children_.back().get();
}

} // namespace blkhurst
