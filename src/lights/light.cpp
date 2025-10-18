#include <blkhurst/lights/light.hpp>

namespace blkhurst {

Light::Light(LightType type)
    : type_(type) {
}

LightType Light::type() const {
  return type_;
}

const glm::vec3& Light::color() const {
  return color_;
}

float Light::intensity() const {
  return intensity_;
}

void Light::setColor(const glm::vec3& color) {
  color_ = color;
}

void Light::setIntensity(float intensity) {
  intensity_ = intensity;
}

} // namespace blkhurst
