#pragma once

#ifdef BLKHURST_ENABLE_OPTIX

#include <blkhurst/cameras/perspective_camera.hpp>
#include <blkhurst/controllers/orbit_controller.hpp>
#include <blkhurst/engine/root_state.hpp>
#include <blkhurst/geometry/box_geometry.hpp>
#include <blkhurst/geometry/sphere_geometry.hpp>
#include <blkhurst/helpers/optix_pass_ui.hpp>
#include <blkhurst/helpers/renderer_ui.hpp>
#include <blkhurst/integrations/optix_pass.hpp>
#include <blkhurst/lights/ambient_light.hpp>
#include <blkhurst/materials/pbr_material.hpp>
#include <blkhurst/postprocessing/effect_composer.hpp>
#include <blkhurst/renderer/renderer.hpp>
#include <blkhurst/scene/scene.hpp>

#include <glm/gtc/quaternion.hpp>

namespace blkhurst {

class OptixPathTracingScene final : public Scene {
public:
  OptixPathTracingScene() {
    auto camera = PerspectiveCamera::create(45.0F, 16.0F / 9.0F, 0.1F, 100.0F);
    camera->setPosition({0.0F, 1.35F, 4.2F});
    camera->lookAt({0.0F, 1.0F, 0.0F});

    setActiveCamera(camera);
    setActiveController(OrbitController::create());
    addUiEntry(RendererUi::create());
    optixPassUi_ = addUiEntry(OptixPassUi::create(nullptr));
  }

  void onStart(const RootState& state) override {
    auto matteWhite = PbrMaterial::create({.color = {0.72F, 0.72F, 0.68F}, .roughness = 0.85F});
    auto matteRed = PbrMaterial::create({.color = {0.75F, 0.12F, 0.08F}, .roughness = 0.85F});
    auto matteGreen = PbrMaterial::create({.color = {0.08F, 0.45F, 0.12F}, .roughness = 0.85F});
    auto matteBlue = PbrMaterial::create({.color = {0.08F, 0.18F, 0.7F}, .roughness = 0.9F});
    auto roughPlastic = PbrMaterial::create({.color = {0.9F, 0.55F, 0.18F}, .roughness = 0.55F});
    auto mirrorMetal =
        PbrMaterial::create({.color = {0.95F, 0.95F, 0.95F}, .metalness = 1.0F, .roughness = 0.0F});
    auto glossyMetal =
        PbrMaterial::create({.color = {0.9F, 0.82F, 0.62F}, .metalness = 1.0F, .roughness = 0.22F});
    auto smoothDielectric =
        PbrMaterial::create({.color = {0.95F, 0.95F, 1.0F}, .metalness = 0.0F, .roughness = 0.0F});
    auto ceilingLight = PbrMaterial::create({
        .color = {1.0F, 1.0F, 1.0F},
        .emissiveColor = {1.0F, 1.0F, 1.0F},
        .emissiveIntensity = 14.0F,
    });
    auto tinyLight = PbrMaterial::create({
        .color = {1.0F, 0.92F, 0.74F},
        .emissiveColor = {1.0F, 0.78F, 0.45F},
        .emissiveIntensity = 55.0F,
    });

    addBox({0.0F, -0.05F, 0.0F}, {3.0F, 0.1F, 3.0F}, matteWhite);
    addBox({0.0F, 2.05F, 0.0F}, {3.0F, 0.1F, 3.0F}, matteWhite);
    addBox({0.0F, 1.0F, -1.5F}, {3.0F, 2.0F, 0.1F}, matteWhite);
    addBox({-1.55F, 1.0F, 0.0F}, {0.1F, 2.0F, 3.0F}, matteRed);
    addBox({1.55F, 1.0F, 0.0F}, {0.1F, 2.0F, 3.0F}, matteGreen);

    auto* shortBox = addBox({-0.62F, 0.35F, -0.5F}, {0.55F, 0.75F, 0.55F}, matteWhite);
    shortBox->setRotation(glm::angleAxis(-0.28F, glm::vec3{0.0F, 1.0F, 0.0F}));
    auto* tallBox = addBox({0.52F, 0.55F, -0.75F}, {0.52F, 1.15F, 0.52F}, matteBlue);
    tallBox->setRotation(glm::angleAxis(0.35F, glm::vec3{0.0F, 1.0F, 0.0F}));

    addSphere({-0.78F, 0.33F, 0.68F}, {0.28F, 0.28F, 0.28F}, mirrorMetal);
    addSphere({0.02F, 0.31F, 0.72F}, {0.26F, 0.26F, 0.26F}, glossyMetal);
    addSphere({0.78F, 0.31F, 0.58F}, {0.26F, 0.26F, 0.26F}, smoothDielectric);
    addSphere({0.95F, 0.16F, -0.05F}, {0.14F, 0.14F, 0.14F}, roughPlastic);

    addBox({0.0F, 1.98F, -0.35F}, {0.42F, 0.035F, 0.42F}, ceilingLight);
    addBox({-1.05F, 1.35F, 0.95F}, {0.08F, 0.08F, 0.08F}, tinyLight);
    addBox({-0.72F, 0.82F, 0.55F}, {0.12F, 0.9F, 0.12F}, matteWhite);

    addChild<AmbientLight>();
  }

  void onAttach(const RootState& state) override {
    auto& renderer = *state.renderer;
    renderer.setToneMappingMode(ToneMappingMode::ACES);
    renderer.setToneMappingExposure(1.0F);

    auto& composer = *state.effectComposer;
    optixPass_ = OptixPass::create(this, activeCamera(),
                                   {
                                       .samplesPerPixel = 1,
                                       .maxBounces = 5,
                                   });
    optixPass_->addTo(composer);
    optixPassUi_->setPass(optixPass_);
  }

private:
  std::shared_ptr<OptixPass> optixPass_;
  std::shared_ptr<OptixPassUi> optixPassUi_;

  Mesh* addBox(const glm::vec3& position, const glm::vec3& scale,
               const std::shared_ptr<PbrMaterial>& material) {
    auto* mesh = addChild<Mesh>(BoxGeometry::create(), material);
    mesh->setPosition(position);
    mesh->setScale(scale);
    return mesh;
  }

  Mesh* addSphere(const glm::vec3& position, const glm::vec3& scale,
                  const std::shared_ptr<PbrMaterial>& material) {
    auto* mesh = addChild<Mesh>(SphereGeometry::create(), material);
    mesh->setPosition(position);
    mesh->setScale(scale);
    return mesh;
  }
};

} // namespace blkhurst

#endif // BLKHURST_ENABLE_OPTIX
