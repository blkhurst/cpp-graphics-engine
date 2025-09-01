#pragma once

#include "cameras/camera.hpp"

namespace blkhurst {

struct PerspectiveDefaults {
  static constexpr float kFovYDeg = 60.0F;
  static constexpr float kAspect = 16.0F / 9.0F;
  static constexpr float kNearZ = 0.1F;
  static constexpr float kFarZ = 1000.0F;
};

class PerspectiveCamera : public Camera {
public:
  PerspectiveCamera();
  PerspectiveCamera(float fovYDeg, float aspect, float nearZ, float farZ);
  ~PerspectiveCamera() override = default;

  PerspectiveCamera(const PerspectiveCamera&) = delete;
  PerspectiveCamera& operator=(const PerspectiveCamera&) = delete;
  PerspectiveCamera(PerspectiveCamera&&) = delete;
  PerspectiveCamera& operator=(PerspectiveCamera&&) = delete;

  void setFovYDeg(float fovYDeg);
  void setAspect(float aspect);
  void setNearFar(float nearZ, float farZ);

  const glm::mat4& projectionMatrix() const override;

private:
  mutable glm::mat4 proj_{1.0F};
  mutable bool projNeedsUpdate_ = true;

  float fovYDeg_ = PerspectiveDefaults::kFovYDeg;
  float aspect_ = PerspectiveDefaults::kAspect;
  float nearZ_ = PerspectiveDefaults::kNearZ;
  float farZ_ = PerspectiveDefaults::kFarZ;
};

} // namespace blkhurst
