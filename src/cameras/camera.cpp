#include <blkhurst/cameras/camera.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

namespace blkhurst {

Camera::Camera()
    : Object3D(NodeType::Camera) {
}

bool Camera::isOrthographic() const {
  return false;
}

glm::mat4 Camera::viewMatrix() const {
  return glm::inverse(worldMatrix());
}

} // namespace blkhurst
