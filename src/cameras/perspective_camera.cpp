#include <blkhurst/cameras/perspective_camera.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

namespace blkhurst {

PerspectiveCamera::PerspectiveCamera() {
  spdlog::trace("PerspectiveCamera({}) constructed (defaults)", uuid());
}

PerspectiveCamera::PerspectiveCamera(float fovYDeg, float aspect, float nearZ, float farZ)
    : fovYDeg_(fovYDeg),
      aspect_(aspect),
      nearZ_(nearZ),
      farZ_(farZ) {
  spdlog::trace("PerspectiveCamera({}) constructed fov={:.2f} aspect={:.2f} near={:.2f} far={:.2f}",
                uuid(), fovYDeg, aspect, nearZ, farZ);
}

PerspectiveCamera::~PerspectiveCamera() {
  spdlog::trace("PerspectiveCamera({}) destroyed", uuid());
}

std::shared_ptr<PerspectiveCamera> PerspectiveCamera::create() {
  return std::make_shared<PerspectiveCamera>();
}

std::shared_ptr<PerspectiveCamera> PerspectiveCamera::create(float fovYDeg, float aspect,
                                                             float nearZ, float farZ) {
  return std::make_shared<PerspectiveCamera>(fovYDeg, aspect, nearZ, farZ);
}

void PerspectiveCamera::onUpdate(const RootState& state) {
  updateAspectFromState(state);
}

const glm::mat4& PerspectiveCamera::projectionMatrix() const {
  if (projNeedsUpdate_) {
    proj_ = glm::perspective(glm::radians(fovYDeg_), aspect_, nearZ_, farZ_);
    projNeedsUpdate_ = false;
  }
  return proj_;
}

void PerspectiveCamera::setFovYDeg(float fovYDeg) {
  fovYDeg_ = fovYDeg;
  projNeedsUpdate_ = true;
  spdlog::trace("PerspectiveCamera setFovYDeg {:.2f}", fovYDeg);
}

void PerspectiveCamera::setAspect(float aspect) {
  aspect_ = aspect;
  projNeedsUpdate_ = true;
  spdlog::trace("PerspectiveCamera setAspect {:.2f}", aspect);
}

void PerspectiveCamera::setNearFar(float nearZ, float farZ) {
  nearZ_ = nearZ;
  farZ_ = farZ;
  projNeedsUpdate_ = true;
  spdlog::trace("PerspectiveCamera setNearFar near={:.2f} far={:.2f}", nearZ, farZ);
}

void PerspectiveCamera::setAutoUpdateAspect(bool enabled) {
  autoUpdateAspect_ = enabled;
}

void PerspectiveCamera::updateAspectFromState(const RootState& state) {
  if (!autoUpdateAspect_ || state.windowFramebufferSize[1] == 0.0F) {
    return;
  }

  float aspect = state.windowFramebufferSize[0] / state.windowFramebufferSize[1];
  if (aspect != aspect_) {
    setAspect(aspect);
  }
}

} // namespace blkhurst
