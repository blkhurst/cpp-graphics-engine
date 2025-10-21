#pragma once
#include <blkhurst/objects/object3d.hpp>
#include <glm/glm.hpp>

namespace blkhurst {

enum class LightType { Ambient, Directional, Point, Rect };

class Light : public Object3D {
public:
  virtual ~Light() = default;

  Light(const Light&) = delete;
  Light& operator=(const Light&) = delete;
  Light(Light&&) = delete;
  Light& operator=(Light&&) = delete;

  [[nodiscard]] LightType type() const;
  [[nodiscard]] const glm::vec3& color() const;
  [[nodiscard]] float intensity() const;

  void setColor(const glm::vec3& color);
  void setIntensity(float intensity);

protected:
  explicit Light(LightType type);

private:
  LightType type_;
  glm::vec3 color_ = glm::vec3(1.0F);
  float intensity_ = 1.0F;
};

} // namespace blkhurst
