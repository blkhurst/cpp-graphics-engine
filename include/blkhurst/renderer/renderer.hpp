#pragma once

#include <blkhurst/cameras/camera.hpp>
#include <blkhurst/engine/config/defaults.hpp>
#include <blkhurst/geometry/geometry.hpp>
#include <blkhurst/graphics/ssbo.hpp>
#include <blkhurst/graphics/ubo.hpp>
#include <blkhurst/ibl/pmrem_generator.hpp>
#include <blkhurst/materials/pipeline_state.hpp>
#include <blkhurst/objects/mesh.hpp>
#include <blkhurst/objects/object3d.hpp>
#include <blkhurst/renderer/cube_render_target.hpp>
#include <blkhurst/renderer/environment_bundle.hpp>
#include <blkhurst/renderer/render_target.hpp>
#include <blkhurst/renderer/uniform_blocks.hpp>

namespace blkhurst {

enum class ToneMappingMode : int { None = 0, Linear = 1, Neutral = 2, ACES = 3 };
enum class OutputColorSpace : int { Linear = 0, SRGB = 1 };

struct FrameContext {
  std::vector<Mesh*> opaqueMeshes;      // Sorted Front-To-Back
  std::vector<Mesh*> transparentMeshes; // Sorted Back-To-Front

  LightDataGPU lightData{};
  std::vector<DirectionalLightGPU> directionalLights;
  std::vector<PointLightGPU> pointLights;
};

struct GpuBlocks {
  UBO frame{uniform_bindings::Frame};
  UBO lightData{uniform_bindings::LightData};
  SSBO directionalLights{uniform_bindings::DirectionalLights};
  SSBO pointLights{uniform_bindings::PointLights};
};

struct RendererDesc {
  const RenderTarget* currentTarget_ = nullptr; // Fix: Does NOT store CubeRenderTarget state

  bool autoClear = true;
  glm::vec4 clearColor = defaults::window::clearColor;
  glm::ivec2 framebufferSize = {0, 0}; // Window Backbuffer

  float toneMappingExposure = 1.0F;
  ToneMappingMode toneMappingMode = ToneMappingMode::None;
  OutputColorSpace outputColorSpace = OutputColorSpace::SRGB;
};

class Renderer {
public:
  Renderer();
  ~Renderer() = default;

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;
  Renderer(Renderer&&) = delete;
  Renderer& operator=(Renderer&&) = delete;

  [[nodiscard]] const RendererDesc& desc() const;

  void render(Object3D& root, Camera& camera);
  void beginPass();

  void setFrameUniforms(const FrameUniforms& frameUniforms); // Used by Engine
  void setDefaultFramebufferSize(int width, int height);     // Set by engine

  void setRenderTarget(const RenderTarget* target);
  void setRenderTarget(const CubeRenderTarget* target, int face, int mip = 0);

  void setAutoClear(bool enabled = true);
  void setClearColor(glm::vec4 rgba);
  void clear(bool color = true, bool depth = true, bool stencil = true);

  void setViewport(int xpos, int ypos, int width, int height);
  void resetViewport();

  void setScissor(int xpos, int ypos, int width, int height);
  void setScissorTest(bool enabled);

  void setToneMappingExposure(float exposure);
  void setToneMappingMode(ToneMappingMode mode);
  void setOutputColorSpace(OutputColorSpace space);

  void resetState();
  // TODO: setAnimationLoop, copyFrameBufferToTexture

private:
  RendererDesc desc_{};

  FrameUniforms frameUniforms_{};
  GpuBlocks gpuBlocks_{};

  // Queue & Sort visible meshes & lights for this frame
  static FrameContext collectRenderables(Object3D& root, const Camera& camera);

  // Render a single mesh (geometry + material)
  void renderMesh(const Mesh& mesh, const Camera& camera);

  // Apply Per-Draw Pipeline State (depth, blend, cull)
  static void applyPipeline(const PipelineState& state, bool wireframe);

  //
  static void drawGeometry(const Geometry& geom, int instanceCount);

  //
  void applyPerFrameUniforms(const FrameContext& frameContext);

  //
  void applyPerDrawUniforms(const Mesh& mesh, Material& material) const;

  //
  static void applyInstancing(const Mesh& mesh, Geometry& geometry, Material& material);

  // ------ Background / Environment ------

  //
  std::unique_ptr<Mesh> skyboxMesh_;
  //
  void renderBackground(Scene& scene, Camera& camera);

  //
  PMREMGenerator pmremGenerator_{this};
  //
  EnvironmentBundle environmentBundle_{};
  //
  void setEnvironment(Scene& scene);

  // ------ Utilities ------

  //
  static unsigned toGlPrimitive(PrimitiveMode mode);
  //
  static unsigned toGLDepthFunc(blkhurst::DepthFunc func);
};

} // namespace blkhurst
