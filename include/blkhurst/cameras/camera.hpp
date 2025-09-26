#pragma once

#include <blkhurst/objects/object3d.hpp>
#include <glm/glm.hpp>

namespace blkhurst {

class Camera : public Object3D {
public:
  Camera() = default;
  ~Camera() override = default;

  Camera(const Camera&) = delete;
  Camera& operator=(const Camera&) = delete;
  Camera(Camera&&) = delete;
  Camera& operator=(Camera&&) = delete;

  NodeKind kind() const override {
    return NodeKind::Camera;
  }

  void onUpdate(const RootState& state) override {
  }

  glm::mat4 viewMatrix() const;
  virtual bool isOrthographic() const;
  virtual const glm::mat4& projectionMatrix() const = 0;
};

} // namespace blkhurst
