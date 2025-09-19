#pragma once

#include <blkhurst/cameras/camera.hpp>
#include <blkhurst/engine/config/defaults.hpp>
#include <blkhurst/geometry/geometry.hpp>
#include <blkhurst/materials/pipeline_state.hpp>
#include <blkhurst/objects/mesh.hpp>
#include <blkhurst/objects/object3d.hpp>
#include <blkhurst/renderer/render_target.hpp>
#include <blkhurst/renderer/uniforms.hpp>

namespace blkhurst {

class Renderer {
public:
  Renderer() = default;
  ~Renderer() = default;

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;
  Renderer(Renderer&&) = delete;
  Renderer& operator=(Renderer&&) = delete;

  void setFrameUniforms(const FrameUniforms& frameUniforms); // Used by Engine
  void setRenderTarget(const RenderTarget* target);
  void render(Object3D& root, Camera& camera);

  void setAutoClear(bool enabled = true);
  void setClearColor(glm::vec4 rgba);
  void clear(bool color = true, bool depth = true, bool stencil = true);
  void clearColor();
  void clearDepth();
  void clearStencil();

  void setDefaultFramebufferSize(int width, int height); // Set by engine
  void setViewport(int xpos, int ypos, int width, int height);
  void setScissor(int xpos, int ypos, int width, int height);
  void setScissorTest(bool enabled);
  // TODO: setAnimationLoop, copyFrameBufferToTexture

  void resetState();

private:
  FrameUniforms frameUniforms_{};
  const RenderTarget* currentTarget_ = nullptr;

  bool autoClear_ = true;
  bool scissorTestEnabled_ = false;

  glm::vec4 clearColor_ = defaults::window::clearColor;

  glm::ivec2 framebufferSize_ = {0, 0}; // Window Backbuffer

  void renderMesh(const Mesh& mesh, const Camera& camera);
  static void applyPipeline(const PipelineState& state, bool wireframe);
  void applyPerFrameUniforms(const Camera& camera);
  void applyPerDrawUniforms(const Mesh& mesh, const Camera& camera) const;
  static void drawGeometry(const Geometry& geom, int instanceCount);

  // Helpers
  static unsigned toGlPrimitive(PrimitiveMode mode);
  static unsigned toGLDepthFunc(blkhurst::DepthFunc func);
};

} // namespace blkhurst
