#include <blkhurst/cameras/ortho_camera.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

namespace blkhurst {

OrthoCamera::OrthoCamera() {
  spdlog::trace("OrthoCamera({}) constructed (defaults)", uuid());
}

OrthoCamera::OrthoCamera(float left, float right, float bottom, float top, float nearZ, float farZ)
    : left_(left),
      right_(right),
      bottom_(bottom),
      top_(top),
      nearZ_(nearZ),
      farZ_(farZ) {
  spdlog::trace(
      "OrthoCamera({}) constructed lrtb=({:.2f},{:.2f},{:.2f},{:.2f}) near={:.2f} far={:.2f}",
      uuid(), left, right, bottom, top, nearZ, farZ);
}

OrthoCamera::~OrthoCamera() {
  spdlog::trace("OrthoCamera({}) destroyed", uuid());
}

std::shared_ptr<OrthoCamera> OrthoCamera::create() {
  return std::make_shared<OrthoCamera>();
}

std::shared_ptr<OrthoCamera> OrthoCamera::create(float left, float right, float bottom, float top,
                                                 float nearZ, float farZ) {
  return std::make_shared<OrthoCamera>(left, right, bottom, top, nearZ, farZ);
}

bool OrthoCamera::isOrthographic() const {
  return true;
}

const glm::mat4& OrthoCamera::projectionMatrix() const {
  if (projNeedsUpdate_) {
    proj_ = glm::ortho(left_, right_, bottom_, top_, nearZ_, farZ_);
    projNeedsUpdate_ = false;
  }
  return proj_;
}

void OrthoCamera::setBounds(float left, float right, float bottom, float top) {
  left_ = left;
  right_ = right;
  bottom_ = bottom;
  top_ = top;
  projNeedsUpdate_ = true;
  spdlog::trace("OrthoCamera setBounds lrtb=({:.2f},{:.2f},{:.2f},{:.2f})", left, right, bottom,
                top);
}

void OrthoCamera::setNearFar(float nearZ, float farZ) {
  nearZ_ = nearZ;
  farZ_ = farZ;
  projNeedsUpdate_ = true;
  spdlog::trace("OrthoCamera setNearFar near={:.2f} far={:.2f}", nearZ, farZ);
}

std::unique_ptr<OrthoCamera> OrthoCamera::clone(bool recursive) {
  auto copy = std::make_unique<OrthoCamera>();
  // Copy Object3D state
  copy->setName(name());
  copy->setVisible(visible());
  copy->setPosition(position());
  copy->setRotation(rotation());
  copy->setScale(scale());
  // Copy PerspectiveCamera state
  copy->left_ = left_;
  copy->right_ = right_;
  copy->bottom_ = bottom_;
  copy->top_ = top_;
  copy->nearZ_ = nearZ_;
  copy->farZ_ = farZ_;
  copy->projNeedsUpdate_ = true; // Force update

  if (recursive) {
    for (const auto& child : children()) {
      copy->addChild_(child->clone(true));
    }
  }
  return copy;
}

} // namespace blkhurst
