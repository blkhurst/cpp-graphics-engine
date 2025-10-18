#pragma once
#include <blkhurst/lights/light.hpp>

namespace blkhurst {

class PointLight : public Light {
public:
  PointLight();

  [[nodiscard]] float decay() const;
  [[nodiscard]] float distance() const;
  [[nodiscard]] float power() const; // Power in lumens (lm)

  void setDecay(float decay);
  void setDistance(float distance);
  void setPower(float lumens);

private:
  // Attenuation exponent, 2.0 is physically correct inverse square falloff
  float decay_ = 2.0F;
  // Range in world units (0 = infinite)
  float distance_ = 0.0F;
};

} // namespace blkhurst
