// NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
#include <blkhurst/cameras/perspective_camera.hpp>
#include <blkhurst/controllers/orbit_controller.hpp>
#include <blkhurst/input/input.hpp>
#include <glm/gtc/constants.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <spdlog/spdlog.h>

namespace {
constexpr float kTwoPi = glm::two_pi<float>();
constexpr float kEpsilon = 1e-6F;
constexpr float fovFallback = 50.0F;
constexpr glm::vec2 scrollDeltaMultiplier{100.0F}; // GLFW scrollCallback returns +1 -1
} // namespace

namespace blkhurst {

OrbitController::OrbitController(const OrbitControllerDesc& desc)
    : target_(desc.target),
      radius_(desc.radius),
      polar_(desc.polar),
      azimuth_(desc.azimuth),
      minRadius_(desc.minRadius),
      maxRadius_(desc.maxRadius),
      minPolar_(desc.minPolar),
      maxPolar_(desc.maxPolar),
      minAzimuth_(desc.minAzimuth),
      maxAzimuth_(desc.maxAzimuth),
      rotateSpeed_(desc.rotateSpeed),
      panSpeed_(desc.panSpeed),
      zoomSpeed_(desc.zoomSpeed),
      dampingEnabled_(desc.dampingEnabled),
      dampingFactor_(desc.dampingFactor),
      panEnabled_(desc.panEnabled),
      zoomEnabled_(desc.zoomEnabled),
      rotateEnabled_(desc.rotateEnabled),
      autoRotate_(desc.autoRotate),
      autoRotateSpeed_(desc.autoRotateSpeed),
      worldSpacePanning_(desc.worldSpacePanning) {
  setSpherical(radius_, polar_, azimuth_);
}

void OrbitController::setTarget(const glm::vec3& target) {
  target_ = target;
}
const glm::vec3& OrbitController::target() const {
  return target_;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void OrbitController::setSpherical(float radius, float polar, float azimuth) {
  radius_ = glm::clamp(radius, minRadius_, maxRadius_);
  polar_ = glm::clamp(polar, minPolar_, maxPolar_);
  azimuth_ = glm::clamp(wrapPi(azimuth), minAzimuth_, maxAzimuth_);
}

void OrbitController::setRotateSpeed(float speed) {
  rotateSpeed_ = speed;
}
void OrbitController::setPanSpeed(float speed) {
  panSpeed_ = speed;
}
void OrbitController::setZoomSpeed(float speed) {
  zoomSpeed_ = speed;
}

void OrbitController::setDistanceLimits(float minDist, float maxDist) {
  minRadius_ = std::max(0.0F, std::min(minDist, maxDist));
  maxRadius_ = std::max(minDist, maxDist);
  radius_ = glm::clamp(radius_, minRadius_, maxRadius_);
}
void OrbitController::setPolarLimits(float minPolarRad, float maxPolarRad) {
  minPolar_ = std::min(minPolarRad, maxPolarRad);
  maxPolar_ = std::max(minPolarRad, maxPolarRad);
  polar_ = glm::clamp(polar_, minPolar_, maxPolar_);
}
void OrbitController::setAzimuthalLimits(float minAzimuthRad, float maxAzimuthRad) {
  minAzimuth_ = std::min(minAzimuthRad, maxAzimuthRad);
  maxAzimuth_ = std::max(minAzimuthRad, maxAzimuthRad);
  azimuth_ = glm::clamp(azimuth_, minAzimuth_, maxAzimuth_);
}

void OrbitController::setDampingEnabled(bool enabled) {
  dampingEnabled_ = enabled;
}
void OrbitController::setDampingFactor(float factor) {
  dampingFactor_ = glm::clamp(factor, 0.0F, 1.0F);
}

void OrbitController::enablePan(bool enabled) {
  panEnabled_ = enabled;
}
void OrbitController::enableZoom(bool enabled) {
  zoomEnabled_ = enabled;
}
void OrbitController::enableRotate(bool enabled) {
  rotateEnabled_ = enabled;
}

void OrbitController::setAutoRotate(bool enabled) {
  autoRotate_ = enabled;
}
void OrbitController::setAutoRotateSpeed(float speed) {
  autoRotateSpeed_ = speed;
}
void OrbitController::setWorldSpacePanning(bool enabled) {
  worldSpacePanning_ = enabled;
}

/* --------------------------------- Update ---------------------------------- */

void OrbitController::update(const RootState& state) {
  if (state.camera == nullptr || state.input == nullptr) {
    return;
  }

  const FrameSample sample = sampleFrame(state);

  accumulateRotate(sample);
  accumulatePan(sample);
  accumulateZoom(sample);

  integrateWithDamping();

  clampSpherical();

  applyCameraTransform(*state.camera, target_, radius_, polar_, azimuth_);
}

OrbitController::FrameSample OrbitController::sampleFrame(const RootState& state) {
  FrameSample sample{};
  sample.dt = std::max(state.delta, 0.0F);
  sample.vpW = std::max(state.windowFramebufferSize[0], 1.0F);
  sample.vpH = std::max(state.windowFramebufferSize[1], 1.0F);

  sample.mouseDelta = state.input->mouseDelta();
  sample.scrollDelta = state.input->scrollDelta() * scrollDeltaMultiplier;

  sample.leftDown = state.input->mouseDown(MouseButton::Left);
  sample.middleDown = state.input->mouseDown(MouseButton::Middle);
  sample.rightDown = state.input->mouseDown(MouseButton::Right);
  return sample;
}

// Mouse (LMB) + optional auto-rotate -> angularDelta_
void OrbitController::accumulateRotate(const FrameSample& frm) {
  if (autoRotate_ && !frm.leftDown && !frm.middleDown && !frm.rightDown) {
    const float autoAngle = (kTwoPi / 60.0F) * autoRotateSpeed_ * frm.dt;
    angularDelta_.x -= autoAngle;
  }
  if (rotateEnabled_ && frm.leftDown && !(frm.middleDown || frm.rightDown)) {
    const float factor = (kTwoPi / frm.vpH) * rotateSpeed_;
    angularDelta_.x -= frm.mouseDelta.x * factor; // azimuth
    angularDelta_.y -= frm.mouseDelta.y * factor; // polar
  }
}

// Mid/RMB panning in WorldUp or CameraUp frame -> panOffset_
void OrbitController::accumulatePan(const FrameSample& frm) {
  if (!panEnabled_ || !(frm.middleDown || frm.rightDown)) {
    return;
  }

  const glm::vec3 camPos = sphericalToCartesian(radius_, polar_, azimuth_) + target_;
  const glm::vec3 forward = glm::normalize(target_ - camPos);

  // TODO: Implement PerspectiveCamera::FovGetter and Camera::isPerspective
  // Three-like normalization for perspective: 2 * dist * tan(fov/2) / viewportHeight
  float fovY = glm::radians(fovFallback);
  // Fetch PerspectiveCameras FOV
  const float dist = glm::length(camPos - target_);
  const float norm = (2.0F * dist * std::tan(0.5F * fovY)) / frm.vpH;
  const float deltaX = frm.mouseDelta.x * norm * panSpeed_;
  const float deltaY = frm.mouseDelta.y * norm * panSpeed_;

  if (worldSpacePanning_) {
    // Lock to XZ plane
    const float sinA = std::sin(azimuth_);
    const float cosA = std::cos(azimuth_);
    const glm::vec3 rightXZ{cosA, 0.0F, -sinA};
    const glm::vec3 fwdXZ{sinA, 0.0F, cosA};
    panOffset_ += (-deltaX) * rightXZ;
    panOffset_ += (deltaY)*fwdXZ;
  } else {
    // Screen-space pan using camera right/up
    glm::vec3 worldUp{0.0F, 1.0F, 0.0F};
    glm::vec3 right = glm::cross(forward, worldUp);
    if (glm::length2(right) < kEpsilon) {
      right = {std::cos(azimuth_), 0.0F, -std::sin(azimuth_)};
    } else {
      right = glm::normalize(right);
    }

    const glm::vec3 upVec = glm::normalize(glm::cross(right, forward));
    panOffset_ += (-deltaX) * right;
    panOffset_ += (deltaY)*upVec;
  }
}

// Scroll -> multiplicative zoom scale
void OrbitController::accumulateZoom(const FrameSample& frm) {
  if (!zoomEnabled_ || frm.scrollDelta.y == 0.0F) {
    return;
  }
  const float scale = threeZoomScale(frm.scrollDelta.y, zoomSpeed_);
  zoomScale_ = (frm.scrollDelta.y < 0.0F) ? (zoomScale_ / scale) : (zoomScale_ * scale);
}

// Enforce limits and clamp
void OrbitController::clampSpherical() {
  azimuth_ = glm::clamp(wrapPi(azimuth_), minAzimuth_, maxAzimuth_);
  const float eps = kEpsilon;
  polar_ =
      glm::clamp(polar_, std::max(minPolar_, eps), std::min(maxPolar_, glm::pi<float>() - eps));
  radius_ = glm::clamp(radius_, minRadius_, maxRadius_);
}

// Integrate deltas with optional damping; update radius/target; decay integrators
void OrbitController::integrateWithDamping() {
  if (dampingEnabled_) {
    // integrate angles with a fraction of the delta each frame
    azimuth_ += angularDelta_.x * dampingFactor_;
    polar_ += angularDelta_.y * dampingFactor_;

    // integrate zoom smoothly by applying a fractional exponent
    const float applied = std::pow(zoomScale_, dampingFactor_);
    radius_ = glm::clamp(radius_ * applied, minRadius_, maxRadius_);
    const float keep = 1.0F - dampingFactor_;
    zoomScale_ = std::pow(zoomScale_, keep);
    if (std::abs(zoomScale_ - 1.0F) < kEpsilon) {
      zoomScale_ = 1.0F;
    }

    // integrate pan toward target
    target_ += panOffset_ * dampingFactor_;

    // decay integrators
    const float decay = (1.0F - dampingFactor_);
    angularDelta_ *= decay;
    panOffset_ *= decay;
  } else {
    azimuth_ += angularDelta_.x;
    polar_ += angularDelta_.y;
    radius_ = glm::clamp(radius_ * zoomScale_, minRadius_, maxRadius_);
    target_ += panOffset_;
    angularDelta_ = {0.0F, 0.0F};
    panOffset_ = {0.0F, 0.0F, 0.0F};
    zoomScale_ = 1.0F;
  }
}

// Compute camera position from spherical and point it at target
void OrbitController::applyCameraTransform(Camera& cam, const glm::vec3& target, float radius,
                                           float polar, float azimuth) {
  const glm::vec3 pos = sphericalToCartesian(radius, polar, azimuth) + target;
  cam.setPosition(pos);
  cam.lookAt(target);
}

/* --------------------------------- Helpers --------------------------------- */

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
glm::vec3 OrbitController::sphericalToCartesian(float r, float phi, float theta) {
  const float sinP = std::sin(phi);
  const float cosP = std::cos(phi);
  const float sinA = std::sin(theta);
  const float cosA = std::cos(theta);
  return {r * sinP * sinA,  // x
          r * cosP,         // y
          r * sinP * cosA}; // z
}

float OrbitController::wrapPi(float azimuth) {
  if (azimuth > kTwoPi) {
    azimuth = std::fmod(azimuth, kTwoPi);
  }
  if (azimuth < -kTwoPi) {
    azimuth = std::fmod(azimuth, -kTwoPi);
  }
  if (azimuth > glm::pi<float>()) {
    azimuth -= kTwoPi;
  }
  if (azimuth < -glm::pi<float>()) {
    azimuth += kTwoPi;
  }
  return azimuth;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
float OrbitController::threeZoomScale(float deltaY, float zoomSpeed) {
  // Three: normalized = |delta * 0.01|; scale = pow(0.95, zoomSpeed * normalized)
  const float normalized = std::abs(deltaY * 0.01F);
  const float scale = std::pow(0.95F, zoomSpeed * normalized);
  return (scale > 0.0F) ? scale : 1.0F;
}

} // namespace blkhurst

// NOLINTEND(cppcoreguidelines-pro-type-union-access)
