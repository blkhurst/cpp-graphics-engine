#include "cameras/camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

namespace blkhurst {

glm::mat4 Camera::viewMatrix() const {
  return glm::inverse(worldMatrix());
}

} // namespace blkhurst
