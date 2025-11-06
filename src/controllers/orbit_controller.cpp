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
    : desc_(desc) {
  setSpherical(desc.radius, desc.polar, desc.azimuth);
}

std::shared_ptr<OrbitController> OrbitController::create(const OrbitControllerDesc& desc) {
  return std::make_shared<OrbitController>(desc);
}

const OrbitControllerDesc& OrbitController::desc() const {
  return desc_;
}

void OrbitController::setTarget(const glm::vec3& target) {
  desc_.target = target;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void OrbitController::setSpherical(float radius, float polar, float azimuth) {
  desc_.radius = glm::clamp(radius, desc_.minRadius, desc_.maxRadius);
  desc_.polar = glm::clamp(polar, desc_.minPolar, desc_.maxPolar);
  desc_.azimuth = glm::clamp(wrapPi(azimuth), desc_.minAzimuth, desc_.maxAzimuth);
}

void OrbitController::setRotateSpeed(float speed) {
  desc_.rotateSpeed = speed;
}
void OrbitController::setPanSpeed(float speed) {
  desc_.panSpeed = speed;
}
void OrbitController::setZoomSpeed(float speed) {
  desc_.zoomSpeed = speed;
}

void OrbitController::setDistanceLimits(float minDist, float maxDist) {
  desc_.minRadius = std::max(0.0F, std::min(minDist, maxDist));
  desc_.maxRadius = std::max(minDist, maxDist);
  desc_.radius = glm::clamp(desc_.radius, desc_.minRadius, desc_.maxRadius);
}
void OrbitController::setPolarLimits(float minPolarRad, float maxPolarRad) {
  desc_.minPolar = std::min(minPolarRad, maxPolarRad);
  desc_.maxPolar = std::max(minPolarRad, maxPolarRad);
  desc_.polar = glm::clamp(desc_.polar, desc_.minPolar, desc_.maxPolar);
}
void OrbitController::setAzimuthalLimits(float minAzimuthRad, float maxAzimuthRad) {
  desc_.minAzimuth = std::min(minAzimuthRad, maxAzimuthRad);
  desc_.maxAzimuth = std::max(minAzimuthRad, maxAzimuthRad);
  desc_.azimuth = glm::clamp(desc_.azimuth, desc_.minAzimuth, desc_.maxAzimuth);
}

void OrbitController::setDampingEnabled(bool enabled) {
  desc_.dampingEnabled = enabled;
}
void OrbitController::setDampingFactor(float factor) {
  desc_.dampingFactor = glm::clamp(factor, 0.0F, 1.0F);
}

void OrbitController::enablePan(bool enabled) {
  desc_.panEnabled = enabled;
}
void OrbitController::enableZoom(bool enabled) {
  desc_.zoomEnabled = enabled;
}
void OrbitController::enableRotate(bool enabled) {
  desc_.rotateEnabled = enabled;
}

void OrbitController::setAutoRotate(bool enabled) {
  desc_.autoRotate = enabled;
}
void OrbitController::setAutoRotateSpeed(float speed) {
  desc_.autoRotateSpeed = speed;
}
void OrbitController::setWorldSpacePanning(bool enabled) {
  desc_.worldSpacePanning = enabled;
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

  integrateWithDamping(sample.dt);

  clampSpherical();

  applyCameraTransform(*state.camera, desc_.target, desc_.radius, desc_.polar, desc_.azimuth);
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
  if (desc_.autoRotate && !frm.leftDown && !frm.middleDown && !frm.rightDown) {
    const float autoAngle = (kTwoPi / 60.0F) * desc_.autoRotateSpeed * frm.dt;
    angularDelta_.x -= autoAngle;
  }
  if (desc_.rotateEnabled && frm.leftDown && !(frm.middleDown || frm.rightDown)) {
    const float factor = (kTwoPi / frm.vpH) * desc_.rotateSpeed;
    angularDelta_.x -= frm.mouseDelta.x * factor; // azimuth
    angularDelta_.y -= frm.mouseDelta.y * factor; // polar
  }
}

// Mid/RMB panning in WorldUp or CameraUp frame -> panOffset_
void OrbitController::accumulatePan(const FrameSample& frm) {
  if (!desc_.panEnabled || !(frm.middleDown || frm.rightDown)) {
    return;
  }

  const glm::vec3 camPos =
      sphericalToCartesian(desc_.radius, desc_.polar, desc_.azimuth) + desc_.target;
  const glm::vec3 forward = glm::normalize(desc_.target - camPos);

  // TODO: Implement PerspectiveCamera::FovGetter and Camera::isPerspective
  // Three-like normalization for perspective: 2 * dist * tan(fov/2) / viewportHeight
  float fovY = glm::radians(fovFallback);
  // Fetch PerspectiveCameras FOV
  const float dist = glm::length(camPos - desc_.target);
  const float norm = (2.0F * dist * std::tan(0.5F * fovY)) / frm.vpH;
  const float deltaX = frm.mouseDelta.x * norm * desc_.panSpeed;
  const float deltaY = frm.mouseDelta.y * norm * desc_.panSpeed;

  if (desc_.worldSpacePanning) {
    // Lock to XZ plane
    const float sinA = std::sin(desc_.azimuth);
    const float cosA = std::cos(desc_.azimuth);
    const glm::vec3 rightXZ{cosA, 0.0F, -sinA};
    const glm::vec3 fwdXZ{sinA, 0.0F, cosA};
    panOffset_ += (-deltaX) * rightXZ;
    panOffset_ += (deltaY)*fwdXZ;
  } else {
    // Screen-space pan using camera right/up
    glm::vec3 worldUp{0.0F, 1.0F, 0.0F};
    glm::vec3 right = glm::cross(forward, worldUp);
    if (glm::length2(right) < kEpsilon) {
      right = {std::cos(desc_.azimuth), 0.0F, -std::sin(desc_.azimuth)};
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
  if (!desc_.zoomEnabled || frm.scrollDelta.y == 0.0F) {
    return;
  }
  const float scale = threeZoomScale(frm.scrollDelta.y, desc_.zoomSpeed);
  zoomScale_ = (frm.scrollDelta.y < 0.0F) ? (zoomScale_ / scale) : (zoomScale_ * scale);
}

// Enforce limits and clamp
void OrbitController::clampSpherical() {
  desc_.azimuth = glm::clamp(wrapPi(desc_.azimuth), desc_.minAzimuth, desc_.maxAzimuth);
  const float eps = kEpsilon;
  desc_.polar = glm::clamp(desc_.polar, std::max(desc_.minPolar, eps),
                           std::min(desc_.maxPolar, glm::pi<float>() - eps));
  desc_.radius = glm::clamp(desc_.radius, desc_.minRadius, desc_.maxRadius);
}

static float alphaFromDeltaHalfLife(float delta, float t_half) {
  if (t_half <= 0.0F || delta <= 0.0F) {
    return 1.0F;
  }
  return 1.0F - std::pow(0.5F, delta / t_half);
}

// Integrate deltas with optional damping; update radius/target; decay integrators
void OrbitController::integrateWithDamping(float delta) {
  if (desc_.dampingEnabled) {
    const float alpha = alphaFromDeltaHalfLife(delta, desc_.dampingFactor);
    const float keep = 1.0F - alpha;

    // integrate angles with a fraction of the delta each frame (now time-based)
    desc_.azimuth += angularDelta_.x * alpha;
    desc_.polar += angularDelta_.y * alpha;

    // integrate zoom smoothly by applying a fractional exponent
    const float applied = std::pow(zoomScale_, alpha);
    desc_.radius = glm::clamp(desc_.radius * applied, desc_.minRadius, desc_.maxRadius);
    //
    zoomScale_ = std::pow(zoomScale_, keep);
    if (std::abs(zoomScale_ - 1.0F) < kEpsilon) {
      zoomScale_ = 1.0F;
    }

    // integrate pan toward target
    desc_.target += panOffset_ * alpha;

    // decay integrators
    angularDelta_ *= keep;
    panOffset_ *= keep;
  } else {
    desc_.azimuth += angularDelta_.x;
    desc_.polar += angularDelta_.y;
    desc_.radius = glm::clamp(desc_.radius * zoomScale_, desc_.minRadius, desc_.maxRadius);
    desc_.target += panOffset_;
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
