#pragma once

#include <glm/glm.hpp>

namespace blkhurst::color {

float srgbToLinear(float value) noexcept;
glm::vec3 srgbToLinear(const glm::vec3& value) noexcept;
glm::vec4 srgbToLinear(const glm::vec4& value) noexcept;

} // namespace blkhurst::color
