// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
#pragma once

#include <blkhurst/cameras/camera.hpp>
#include <blkhurst/controllers/controller.hpp>
#include <blkhurst/engine/root_state.hpp>
#include <glm/glm.hpp>
#include <limits>

namespace blkhurst {

struct OrbitControllerDesc {
  glm::vec3 target{0.0F, 0.0F, 0.0F};

  // Spherical state (radius r, polar φ [0..π], azimuth θ [-π..π])
  float radius{5.0F};
  float polar{1.0F};
  float azimuth{0.0F};

  // Normalised speeds
  float rotateSpeed{1.0F}; // Scaled by viewport height
  float panSpeed{1.0F};    // Normalised by dist & fov
  float zoomSpeed{2.0F};   // pow(0.95, zoomSpeed * delta)

  // Limits
  float minRadius{0.2F};
  float maxRadius{200.0F};
  float minPolar{0.0F};
  float maxPolar{glm::pi<float>()};
  float minAzimuth{-std::numeric_limits<float>::infinity()};
  float maxAzimuth{std::numeric_limits<float>::infinity()};

  // Damping [0-1]
  bool dampingEnabled{true};
  float dampingFactor{0.1F};

  // Feature toggles
  bool panEnabled{true};
  bool zoomEnabled{true};
  bool rotateEnabled{true};

  // Auto-rotate ((2pi/60) * speed * dt)
  bool autoRotate{false};
  float autoRotateSpeed{2.0F};

  // MapControls
  bool worldSpacePanning{false};
};

// Inspired by Three.js' OrbitControls
class OrbitController final : public Controller {
public:
  explicit OrbitController(const OrbitControllerDesc& desc = {});
  ~OrbitController() override = default;

  OrbitController(const OrbitController&) = delete;
  OrbitController& operator=(const OrbitController&) = delete;
  OrbitController(OrbitController&&) = delete;
  OrbitController& operator=(OrbitController&&) = delete;

  void update(const RootState& state) override;

  void setTarget(const glm::vec3& target);
  [[nodiscard]] const glm::vec3& target() const;

  void setSpherical(float radius, float polar, float azimuth);

  void setRotateSpeed(float speed);
  void setPanSpeed(float speed);
  void setZoomSpeed(float speed);

  void setDistanceLimits(float minDist, float maxDist);
  void setPolarLimits(float minPolarRad, float maxPolarRad);
  void setAzimuthalLimits(float minAzimuthRad, float maxAzimuthRad);

  void setDampingEnabled(bool enabled);
  void setDampingFactor(float factor);

  void enablePan(bool enabled);
  void enableZoom(bool enabled);
  void enableRotate(bool enabled);

  void setAutoRotate(bool enabled);
  void setAutoRotateSpeed(float speed);

  void setWorldSpacePanning(bool enabled);

private:
  glm::vec3 target_;
  float radius_;
  float polar_;
  float azimuth_;

  float minRadius_;
  float maxRadius_;
  float minPolar_;
  float maxPolar_;
  float minAzimuth_;
  float maxAzimuth_;

  float rotateSpeed_;
  float panSpeed_;
  float zoomSpeed_;

  bool dampingEnabled_;
  float dampingFactor_;

  bool panEnabled_;
  bool zoomEnabled_;
  bool rotateEnabled_;

  bool autoRotate_;
  float autoRotateSpeed_;
  bool worldSpacePanning_;

  // Accumulated deltas (Applied per-frame, decayed if damping)
  glm::vec2 angularDelta_{0.0F}; // x: azimuth, y: polar
  glm::vec3 panOffset_{0.0F};
  float zoomScale_{1.0F};

  struct FrameSample {
    float dt{0.016F};
    float vpW{1920.0F};
    float vpH{1080.0F};
    glm::vec2 mouseDelta{0.0F};
    glm::vec2 scrollDelta{0.0F};
    bool leftDown{false};
    bool middleDown{false};
    bool rightDown{false};
  };

  static FrameSample sampleFrame(const RootState& state);
  void accumulateRotate(const FrameSample& frm);
  void accumulatePan(const FrameSample& frm);
  void accumulateZoom(const FrameSample& frm);
  void clampSpherical();
  void integrateWithDamping();
  static void applyCameraTransform(Camera& cam, const glm::vec3& target, float radius, float polar,
                                   float azimuth);

  // Helpers
  static glm::vec3 sphericalToCartesian(float r, float phi, float theta); // Right-handed, Y-up
  static float wrapPi(float azimuth);
  static float threeZoomScale(float deltaY, float zoomSpeed);
};

} // namespace blkhurst

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
