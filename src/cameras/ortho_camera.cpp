#include <blkhurst/cameras/ortho_camera.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

namespace blkhurst {

OrthoCamera::OrthoCamera() {
  spdlog::trace("OrthoCamera constructed (defaults)");
}

OrthoCamera::OrthoCamera(float left, float right, float bottom, float top, float nearZ, float farZ)
    : left_(left),
      right_(right),
      bottom_(bottom),
      top_(top),
      nearZ_(nearZ),
      farZ_(farZ) {
  spdlog::trace("OrthoCamera constructed lrtb=({:.3f},{:.3f},{:.3f},{:.3f}) near={:.3f} far={:.3f}",
                left, right, bottom, top, nearZ, farZ);
}

void OrthoCamera::setBounds(float left, float right, float bottom, float top) {
  left_ = left;
  right_ = right;
  bottom_ = bottom;
  top_ = top;
  projNeedsUpdate_ = true;
  spdlog::trace("OrthoCamera setBounds lrtb=({:.3f},{:.3f},{:.3f},{:.3f})", left, right, bottom,
                top);
}

void OrthoCamera::setNearFar(float nearZ, float farZ) {
  nearZ_ = nearZ;
  farZ_ = farZ;
  projNeedsUpdate_ = true;
  spdlog::trace("OrthoCamera setNearFar near={:.3f} far={:.3f}", nearZ, farZ);
}

const glm::mat4& OrthoCamera::projectionMatrix() const {
  if (projNeedsUpdate_) {
    proj_ = glm::ortho(left_, right_, bottom_, top_, nearZ_, farZ_);
    projNeedsUpdate_ = false;
  }
  return proj_;
}

} // namespace blkhurst
