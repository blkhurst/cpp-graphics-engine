#pragma once

#include <blkhurst/cameras/camera.hpp>

namespace blkhurst {

struct OrthoDefaults {
  static constexpr float kLeft = -1.0F;
  static constexpr float kRight = 1.0F;
  static constexpr float kBottom = -1.0F;
  static constexpr float kTop = 1.0F;
  static constexpr float kNearZ = -1.0F;
  static constexpr float kFarZ = 1.0F;
};

class OrthoCamera : public Camera {
public:
  OrthoCamera();
  OrthoCamera(float left, float right, float bottom, float top, float nearZ, float farZ);
  ~OrthoCamera() override;

  OrthoCamera(const OrthoCamera&) = delete;
  OrthoCamera& operator=(const OrthoCamera&) = delete;
  OrthoCamera(OrthoCamera&&) = delete;
  OrthoCamera& operator=(OrthoCamera&&) = delete;

  static std::shared_ptr<OrthoCamera> create();
  static std::shared_ptr<OrthoCamera> create(float left, float right, float bottom, float top,
                                             float nearZ, float farZ);

  void setBounds(float left, float right, float bottom, float top);
  void setNearFar(float nearZ, float farZ);

  const glm::mat4& projectionMatrix() const override;

private:
  mutable glm::mat4 proj_{1.0F};
  mutable bool projNeedsUpdate_ = true;

  float left_ = OrthoDefaults::kLeft;
  float right_ = OrthoDefaults::kRight;
  float bottom_ = OrthoDefaults::kBottom;
  float top_ = OrthoDefaults::kTop;
  float nearZ_ = OrthoDefaults::kNearZ;
  float farZ_ = OrthoDefaults::kFarZ;
};

} // namespace blkhurst
