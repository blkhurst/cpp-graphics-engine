#include <blkhurst/lights/directional_light.hpp>

namespace blkhurst {

DirectionalLight::DirectionalLight()
    : Light(LightType::Directional) {
  // Default position
  setPosition({0, 1, 0});
}

glm::vec3 DirectionalLight::directionToTarget() {
  // Computes Light->Fragment direction
  // Shader flips sign when computing NdotL (Light incoming direction)
  return glm::normalize(targetPosition_ - worldPosition());
}

void DirectionalLight::setTarget(const glm::vec3& target) {
  targetPosition_ = target;
}

} // namespace blkhurst
