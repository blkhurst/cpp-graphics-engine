#include <blkhurst/lights/point_light.hpp>

namespace blkhurst {

PointLight::PointLight()
    : Light(LightType::Point) {
}

float PointLight::decay() const {
  return decay_;
}

float PointLight::distance() const {
  return distance_;
}

float PointLight::power() const {
  // Convert intensity (candela) to power (lumens)
  const float fourPi = 4.0F * glm::pi<float>();
  return intensity() * fourPi;
}

void PointLight::setDecay(float decay) {
  decay_ = decay;
}

void PointLight::setDistance(float distance) {
  distance_ = distance;
}

void PointLight::setPower(float lumens) {
  // Convert power (lumens) to intensity (candela)
  const float fourPi = 4.0F * glm::pi<float>();
  setIntensity(lumens / fourPi);
}

} // namespace blkhurst
