#pragma once

#ifdef BLKHURST_ENABLE_OPTIX

#include <blkhurst/integrations/optix_pass.hpp>
#include <blkhurst/ui/ui_entry.hpp>

#include <imgui.h>

#include <memory>

namespace blkhurst {

class OptixPassUi final : public UiEntry {
public:
  explicit OptixPassUi(std::shared_ptr<OptixPass> pass)
      : pass_(std::move(pass)) {
    setTitle("OptiX");
  }

  static std::shared_ptr<OptixPassUi> create(std::shared_ptr<OptixPass> pass) {
    return std::make_shared<OptixPassUi>(std::move(pass));
  }

  void setPass(std::shared_ptr<OptixPass> pass) {
    pass_ = std::move(pass);
  }

  void onDraw(const RootState&) override {
    if (!pass_) {
      return;
    }

    auto desc = pass_->pathTracerDesc();
    bool usePathTracing = pass_->usePathTracing();
    bool denoise = pass_->denoise();
    bool accumulate = pass_->accumulate();
    bool descChanged = false;

    if (ImGui::Checkbox("Path Trace", &usePathTracing)) {
      pass_->setUsePathTracing(usePathTracing);
    }

    ImGui::BeginDisabled(!usePathTracing);
    if (ImGui::Checkbox("Denoise", &denoise)) {
      pass_->setDenoise(denoise);
    }
    if (ImGui::Checkbox("Accumulate", &accumulate)) {
      pass_->setAccumulate(accumulate);
    }
    if (ImGui::SliderInt("Samples Per Pixel", &desc.samplesPerPixel, 1, 16)) {
      descChanged = true;
    }
    if (ImGui::SliderInt("Bounces", &desc.maxBounces, 1, 12)) {
      descChanged = true;
    }
    if (ImGui::Button("Reset Accumulation")) {
      pass_->resetAccumulation();
    }

    ImGui::SeparatorText("Integrator");
    descChanged = combo("Integrator", desc.integratorMode,
                        "First hit only\0Path tracing\0Path tracing + direct lighting\0Path "
                        "tracing + MIS\0") ||
                  descChanged;
    descChanged = combo("Debug View", desc.debugView,
                        "Beauty\0Normals\0Albedo\0Depth\0Direct\0Indirect\0PDF\0MIS "
                        "weight\0Bounce count\0") ||
                  descChanged;

    ImGui::SeparatorText("Sampling");
    descChanged = combo("Sampling", desc.samplingMode,
                        "Uniform hemisphere\0Cosine hemisphere\0GGX\0GGX VNDF\0") ||
                  descChanged;
    if (ImGui::Checkbox("Russian Roulette", &desc.enableRussianRoulette)) {
      descChanged = true;
    }

    ImGui::SeparatorText("Lighting");
    descChanged = combo("Environment", desc.environmentMode, "Off\0Flat\0HDRI\0") || descChanged;
    if (ImGui::Checkbox("Direct Light Sampling", &desc.enableDirectLighting)) {
      descChanged = true;
    }
    if (ImGui::Checkbox("Shadow Rays", &desc.enableShadowRays)) {
      descChanged = true;
    }
    if (ImGui::Checkbox("Emissive Lights", &desc.enableEmissiveLights)) {
      descChanged = true;
    }
    ImGui::BeginDisabled();
    bool environmentImportanceSampling = false;
    bool powerWeightedLightSelection = false;
    ImGui::Checkbox("Environment Importance Sampling", &environmentImportanceSampling);
    ImGui::Checkbox("Power Weighted Light Selection", &powerWeightedLightSelection);
    ImGui::EndDisabled();

    ImGui::SeparatorText("Materials");
    descChanged = combo("Material Model", desc.materialMode, "Lambert\0PBR GGX\0") || descChanged;
    if (ImGui::Checkbox("Textures", &desc.enableTextures)) {
      descChanged = true;
    }
    if (ImGui::Checkbox("Normal Maps", &desc.enableNormalMaps)) {
      descChanged = true;
    }
    if (ImGui::Checkbox("Alpha / Cutout", &desc.enableAlpha)) {
      descChanged = true;
    }
    if (ImGui::Checkbox("Delta Mirror Lobe", &desc.enableMirrorReflection)) {
      descChanged = true;
    }
    ImGui::BeginDisabled();
    bool glassRefraction = false;
    ImGui::Checkbox("Glass / Refraction", &glassRefraction);
    ImGui::EndDisabled();

    ImGui::SeparatorText("MIS");
    descChanged = combo("Heuristic", desc.misMode, "Off\0Balance\0Power\0") || descChanged;
    ImGui::BeginDisabled();
    bool complementaryLightHitMis = true;
    ImGui::Checkbox("Complementary Light-Hit MIS", &complementaryLightHitMis);
    ImGui::EndDisabled();

    if (descChanged) {
      pass_->setPathTracerDesc(desc);
    }
    ImGui::EndDisabled();
  }

private:
  std::shared_ptr<OptixPass> pass_;

  template <typename T> static bool combo(const char* label, T& value, const char* items) {
    int selected = static_cast<int>(value);
    if (!ImGui::Combo(label, &selected, items)) {
      return false;
    }
    value = static_cast<T>(selected);
    return true;
  }
};

} // namespace blkhurst

#endif // BLKHURST_ENABLE_OPTIX
