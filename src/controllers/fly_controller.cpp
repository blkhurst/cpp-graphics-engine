#include <blkhurst/controllers/fly_controller.hpp>
#include <blkhurst/input/input.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>
#include <spdlog/spdlog.h>

namespace blkhurst {

FlyController::FlyController(const FlyControllerDesc& desc)
    : desc_(desc) {
}

std::shared_ptr<FlyController> FlyController::create(const FlyControllerDesc& desc) {
  return std::make_shared<FlyController>(desc);
}

const FlyControllerDesc& FlyController::desc() const {
  return desc_;
}

void FlyController::setBaseSpeed(float unitsPerSecond) {
  desc_.baseSpeed = unitsPerSecond;
}

void FlyController::setFastMultiplier(float mul) {
  desc_.fastMultiplier = mul;
}

void FlyController::setMouseSensitivity(float radiansPerPixel) {
  desc_.mouseSensitivity = radiansPerPixel;
}

void FlyController::setDamping(float perSecond) {
  desc_.damping = perSecond;
}

void FlyController::update(const RootState& state) {
  if (state.camera == nullptr || state.input == nullptr) {
    return;
  }

  Camera& cam = *state.camera;

  // Lazily derive yaw/pitch from current camera orientation once
  if (!yawPitchInitialised_) {
    initialiseYawPitchFromCamera_(cam);
  }

  const bool rmbPress = state.input->mousePressed(MouseButton::Right);
  const bool rmbRelease = state.input->mouseReleased(MouseButton::Right);

  if (rmbPress) {
    rotating_ = true;
    skipFirstDelta_ = true; // avoid instant jump
    state.input->setCursorMode(CursorMode::Locked);
  }
  if (rmbRelease) {
    rotating_ = false;
    smoothed_ = glm::vec2{0.0F};
    state.input->setCursorMode(CursorMode::Normal);
  }

  if (rotating_) {
    applyMouseLook_(state, cam);
  }

  applyKeyboardMove_(state, cam);
}

void FlyController::initialiseYawPitchFromCamera_(Camera& cam) {
  const glm::vec3 forward = glm::normalize(cam.rotation() * glm::vec3(0.0F, 0.0F, -1.0F));
  yaw_ = std::atan2(-forward[0], -forward[2]);
  pitch_ = std::asin(forward[1]);
  yawPitchInitialised_ = true;
}

void FlyController::applyMouseLook_(const RootState& state, Camera& cam) {
  glm::vec2 mouseDelta = state.input->mouseDelta();
  if (desc_.enableMouseDamping) {
    if (skipFirstDelta_) {
      skipFirstDelta_ = false;
      mouseDelta = {};
    }
    smoothed_ = smoothed_ * (1.0F - desc_.mouseDampingAlpha) + mouseDelta * desc_.mouseDampingAlpha;
    mouseDelta = smoothed_;
  }
  yaw_ -= mouseDelta[0] * desc_.mouseSensitivity;
  pitch_ -= mouseDelta[1] * desc_.mouseSensitivity;

  // Clamp pitch to avoid flipping
  if (pitch_ > kPitchLimit) {
    pitch_ = kPitchLimit;
  }
  if (pitch_ < -kPitchLimit) {
    pitch_ = -kPitchLimit;
  }

  // Rebuild camera orientation from yaw (Y) then pitch (X): R = Ry * Rx
  const glm::quat qYaw = glm::angleAxis(yaw_, glm::vec3(0.0F, 1.0F, 0.0F));
  const glm::quat qPitch = glm::angleAxis(pitch_, glm::vec3(1.0F, 0.0F, 0.0F));
  const glm::quat newRot = glm::normalize(qYaw * qPitch);
  cam.setRotation(newRot);
}

void FlyController::applyKeyboardMove_(const RootState& state, Camera& cam) {
  const float frameDelta = state.delta;
  if (frameDelta <= 0.0F) {
    return;
  }

  const bool fast = state.input->keyDown(Key::LeftShift) || state.input->keyDown(Key::RightShift);

  float speed = desc_.baseSpeed * (fast ? desc_.fastMultiplier : 1.0F);

  float moveX = 0.0F;
  float moveY = 0.0F;
  float moveZ = 0.0F;

  if (state.input->keyDown(Key::W)) {
    moveZ += 1.0F; // forward
  }
  if (state.input->keyDown(Key::S)) {
    moveZ -= 1.0F; // backward
  }
  if (state.input->keyDown(Key::A)) {
    moveX -= 1.0F; // left
  }
  if (state.input->keyDown(Key::D)) {
    moveX += 1.0F; // right
  }
  if (state.input->keyDown(Key::Space)) {
    moveY += 1.0F; // up
  }
  if (state.input->keyDown(Key::LeftControl) || state.input->keyDown(Key::RightControl)) {
    moveY -= 1.0F; // down
  }

  glm::vec3 wishLocal{moveX, moveY, moveZ};
  if (glm::length2(wishLocal) > 0.0F) {
    wishLocal = glm::normalize(wishLocal);
  }

  // Transform local-direction into world space from camera rotation
  const glm::quat quat = cam.rotation();
  const glm::vec3 fwd = quat * glm::vec3(0.0F, 0.0F, -1.0F);
  const glm::vec3 rgt = quat * glm::vec3(1.0F, 0.0F, 0.0F);
  const glm::vec3 upVec = glm::vec3(0.0F, 1.0F, 0.0F);

  const glm::vec3 wishWorld = (rgt * wishLocal[0]) + (upVec * wishLocal[1]) + (fwd * wishLocal[2]);
  const glm::vec3 targetVel = wishWorld * speed;

  // Dampened interpolation: v += (target - v) * (1 - exp(-damping*dt))
  if (desc_.enableDamping) {
    const float alpha = 1.0F - std::exp(-desc_.damping * frameDelta);
    velocity_ += (targetVel - velocity_) * alpha;
  } else {
    velocity_ = targetVel;
  }

  // Update camera position
  glm::vec3 pos = cam.position();
  pos += velocity_ * frameDelta;
  cam.setPosition(pos);
}

} // namespace blkhurst
