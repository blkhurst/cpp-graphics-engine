// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
#pragma once

#include <blkhurst/cameras/camera.hpp>
#include <blkhurst/controllers/controller.hpp>
#include <glm/glm.hpp>

namespace blkhurst {

struct FlyControllerDesc {
  float baseSpeed_ = 4.0F;
  float fastMultiplier = 3.0F;
  float damping = 12.0F;            // per-second
  float mouseSensitivity = 0.0025F; // rad per pixel

  bool enableDamping = true;
  bool enableMouseDamping = true;
  float mouseDampingAlpha = 0.6F; // 0..1 (higher = snappier)
};

class FlyController final : public Controller {
public:
  FlyController(const FlyControllerDesc& desc = {});
  ~FlyController() override = default;

  FlyController(const FlyController&) = delete;
  FlyController& operator=(const FlyController&) = delete;
  FlyController(FlyController&&) = delete;
  FlyController& operator=(FlyController&&) = delete;

  void update(const RootState& state) override;

  void setBaseSpeed(float unitsPerSecond);
  void setFastMultiplier(float mul);
  void setMouseSensitivity(float radiansPerPixel);
  void setDamping(float perSecond);

private:
  const float kPitchLimit = 1.55334303F; // ~89 degrees

  float baseSpeed_;
  float fastMultiplier_;
  float mouseSensitivity_;
  float damping_;

  bool enableDamping_;
  bool enableMouseDamping_;
  float mouseDampingAlpha_;

  // State
  bool rotating_ = false;
  bool skipFirstDelta_ = false; // prevents the initial jolt when RMB is pressed
  bool yawPitchInitialised_ = false;
  glm::vec2 smoothed_{0.0F}; // Mouse

  float yaw_ = 0.0F;
  float pitch_ = 0.0F;
  glm::vec3 velocity_{0.0F};

  void initialiseYawPitchFromCamera_(Camera& cam);
  void applyMouseLook_(const RootState& state, Camera& cam);
  void applyKeyboardMove_(const RootState& state, Camera& cam);
};

} // namespace blkhurst

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
