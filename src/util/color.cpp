#include <blkhurst/util/color.hpp>

namespace blkhurst::color {

constexpr float SRGB_EPSILON = 0.04045F;
constexpr float SRGB_DENOMINATOR = 12.92F;
constexpr float SRGB_OFFSET = 0.055F;
constexpr float SRGB_NUMERATOR = 1.055F;
constexpr float SRGB_GAMMA = 2.4F;

float srgbToLinear(float value) noexcept {
  if (value <= SRGB_EPSILON) {
    return value / SRGB_DENOMINATOR;
  }
  return std::pow((value + SRGB_OFFSET) / SRGB_NUMERATOR, SRGB_GAMMA);
}

glm::vec3 srgbToLinear(const glm::vec3& value) noexcept {
  return {
      srgbToLinear(value[0]),
      srgbToLinear(value[1]),
      srgbToLinear(value[2]),
  };
}

glm::vec4 srgbToLinear(const glm::vec4& value) noexcept {
  return {
      srgbToLinear(value[0]),
      srgbToLinear(value[1]),
      srgbToLinear(value[2]),
      value[3],
  };
}

} // namespace blkhurst::color
