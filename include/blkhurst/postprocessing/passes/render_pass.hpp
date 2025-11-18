#pragma once
#include <blkhurst/postprocessing/pass.hpp>

#include <memory>

namespace blkhurst {

class Object3D;
class Camera;

class RenderPass : public Pass {
public:
  RenderPass(Object3D* root, Camera* camera);
  static std::shared_ptr<RenderPass> create(Object3D* root, Camera* camera);

  void setScene(Object3D* root);
  void setCamera(Camera* camera);

  void render(const PassRenderContext& context) override;

private:
  Object3D* root_ = nullptr;
  Camera* camera_ = nullptr;
};

} // namespace blkhurst
