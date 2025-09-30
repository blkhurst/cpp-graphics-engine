#pragma once
#include <cmath>
#include <glm/glm.hpp>

namespace blkhurst {

class UvTransform {
public:
  UvTransform() = default;

  void setRepeat(glm::vec2 repeat) {
    repeat_ = repeat;
    needsUpdate_ = true;
  }
  void setOffset(glm::vec2 offset) {
    offset_ = offset;
    needsUpdate_ = true;
  }
  void setCenter(glm::vec2 center) {
    center_ = center;
    needsUpdate_ = true;
  }
  void setRotation(float radians) {
    rotation_ = radians;
    needsUpdate_ = true;
  }
  [[nodiscard]] glm::mat3 matrix() {
    if (needsUpdate_) {
      rebuild_();
      needsUpdate_ = false;
    }
    return matrix_;
  }
  [[nodiscard]] bool isDefault() const {
    return (repeat_ == glm::vec2{1.0F} && offset_ == glm::vec2{0.0F} &&
            center_ == glm::vec2{0.5F} && rotation_ == 0.0F);
  }

private:
  glm::vec2 repeat_{1.0F, 1.0F};
  glm::vec2 offset_{0.0F, 0.0F};
  glm::vec2 center_{0.5F, 0.5F};
  float rotation_{0.0F};

  glm::mat3 matrix_{1.0F};
  bool needsUpdate_{true};

  void rebuild_() {
    const float cosTheta = std::cos(rotation_);
    const float sinTheta = std::sin(rotation_);

    const glm::mat3 scaleMat{repeat_[0], 0, 0, 0, repeat_[1], 0, 0, 0, 1};
    const glm::mat3 rotationMat{cosTheta, sinTheta, 0, -sinTheta, cosTheta, 0, 0, 0, 1};
    const glm::mat3 translationMat{1, 0, 0, 0, 1, 0, offset_[0], offset_[1], 1};
    const glm::mat3 centerNegMat{1, 0, 0, 0, 1, 0, -center_[0], -center_[1], 1};
    const glm::mat3 centerMat{1, 0, 0, 0, 1, 0, center_[0], center_[1], 1};

    matrix_ = translationMat * centerMat * rotationMat * scaleMat * centerNegMat;
  }
};

} // namespace blkhurst
