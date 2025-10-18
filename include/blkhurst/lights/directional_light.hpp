#pragma once
#include <blkhurst/lights/light.hpp>

namespace blkhurst {

class DirectionalLight : public Light {
public:
  DirectionalLight();

  [[nodiscard]] glm::vec3 directionToTarget();
  void setTarget(const glm::vec3& target);

private:
  glm::vec3 targetPosition_{0.0F};
};

} // namespace blkhurst
