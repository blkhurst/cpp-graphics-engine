#include "cameras/perspective_camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

namespace blkhurst {

PerspectiveCamera::PerspectiveCamera() {
  spdlog::trace("PerspectiveCamera constructed (defaults)");
}

PerspectiveCamera::PerspectiveCamera(float fovYDeg, float aspect, float nearZ, float farZ)
    : fovYDeg_(fovYDeg),
      aspect_(aspect),
      nearZ_(nearZ),
      farZ_(farZ) {
  spdlog::trace("PerspectiveCamera constructed fov={:.2f} aspect={:.3f} near={:.3f} far={:.1f}",
                fovYDeg, aspect, nearZ, farZ);
}

void PerspectiveCamera::setFovYDeg(float fovYDeg) {
  fovYDeg_ = fovYDeg;
  projNeedsUpdate_ = true;
  spdlog::trace("PerspectiveCamera setFovYDeg {:.2f}", fovYDeg);
}

void PerspectiveCamera::setAspect(float aspect) {
  aspect_ = aspect;
  projNeedsUpdate_ = true;
  spdlog::trace("PerspectiveCamera setAspect {:.3f}", aspect);
}

void PerspectiveCamera::setNearFar(float nearZ, float farZ) {
  nearZ_ = nearZ;
  farZ_ = farZ;
  projNeedsUpdate_ = true;
  spdlog::trace("PerspectiveCamera setNearFar near={:.3f} far={:.1f}", nearZ, farZ);
}

const glm::mat4& PerspectiveCamera::projectionMatrix() const {
  if (projNeedsUpdate_) {
    proj_ = glm::perspective(glm::radians(fovYDeg_), aspect_, nearZ_, farZ_);
    projNeedsUpdate_ = false;
  }
  return proj_;
}

} // namespace blkhurst
