#include <blkhurst/shaders/shader_registry.hpp>
#include <spdlog/spdlog.h>

#include <blkhurst/shaders/builtin/basic.glsl.hpp>
#include <blkhurst/shaders/builtin/equirect.glsl.hpp>
#include <blkhurst/shaders/builtin/fullscreen.glsl.hpp>
#include <blkhurst/shaders/builtin/ibl/brdf_lut.glsl.hpp>
#include <blkhurst/shaders/builtin/ibl/irradiance.glsl.hpp>
#include <blkhurst/shaders/builtin/ibl/prefilter_ggx.glsl.hpp>
#include <blkhurst/shaders/builtin/skybox.glsl.hpp>
#include <blkhurst/shaders/chunks/color_fragment.glsl.hpp>
#include <blkhurst/shaders/chunks/colorspace_fragment.glsl.hpp>
#include <blkhurst/shaders/chunks/common.glsl.hpp>
#include <blkhurst/shaders/chunks/envmap_fragment.glsl.hpp>
#include <blkhurst/shaders/chunks/io_fragment.glsl.hpp>
#include <blkhurst/shaders/chunks/io_vertex.glsl.hpp>
#include <blkhurst/shaders/chunks/normal_fragment.glsl.hpp>
#include <blkhurst/shaders/chunks/pbr_common.glsl.hpp>
#include <blkhurst/shaders/chunks/tonemapping_fragment.glsl.hpp>
#include <blkhurst/shaders/chunks/uniform_common.hpp>

namespace blkhurst {

void ShaderRegistry::registerSource(std::string name, std::string source) {
  auto& reg = map_();
  auto [it, inserted] = reg.insert_or_assign(std::move(name), std::move(source));
  if (inserted) {
    spdlog::debug("ShaderRegistry registered shader '{}'", it->first);
  } else {
    spdlog::warn("ShaderRegistry replaced existing shader '{}'", it->first);
  }
}

bool ShaderRegistry::has(std::string_view name) {
  return map_().find(std::string{name}) != map_().end();
}

std::optional<std::string> ShaderRegistry::find(std::string_view name) {
  auto& reg = map_();
  auto found = reg.find(std::string{name});
  if (found == reg.end()) {
    spdlog::warn("ShaderRegistry shader '{}' not found", name);
    return std::nullopt;
  }
  return found->second;
}

std::unordered_map<std::string, std::string>& ShaderRegistry::map_() {
  static std::unordered_map<std::string, std::string> instance;
  return instance;
}

void ShaderRegistry::registerBuiltinShaders() {
  ShaderRegistry::registerSource("io_vertex", shaders::io_vertex);
  ShaderRegistry::registerSource("io_fragment", shaders::io_fragment);
  ShaderRegistry::registerSource("uniforms_common", shaders::uniforms_common);
  ShaderRegistry::registerSource("normal_fragment", shaders::normal_fragment);
  ShaderRegistry::registerSource("color_fragment", shaders::color_fragment);
  ShaderRegistry::registerSource("envmap_fragment", shaders::envmap_fragment);
  ShaderRegistry::registerSource("tonemapping_fragment", shaders::tonemapping_fragment);
  ShaderRegistry::registerSource("colorspace_fragment", shaders::colorspace_fragment);
  ShaderRegistry::registerSource("common", shaders::common);

  // SkyBoxMaterial
  ShaderRegistry::registerSource("skybox_vert", shaders::skybox_vert);
  ShaderRegistry::registerSource("skybox_frag", shaders::skybox_frag);

  // BasicMaterial
  ShaderRegistry::registerSource("basic_vert", shaders::basic_vert);
  ShaderRegistry::registerSource("basic_frag", shaders::basic_frag);

  // EquirectMaterial
  ShaderRegistry::registerSource("equirect_frag", shaders::equirect_frag);

  // Fullscreen
  ShaderRegistry::registerSource("fullscreen_vert", shaders::fullscreen_vert);

  // IBL
  ShaderRegistry::registerSource("pbr_common", shaders::pbr_common);
  ShaderRegistry::registerSource("brdf_lut_frag", shaders::brdf_lut_frag);
  ShaderRegistry::registerSource("irradiance_frag", shaders::irradiance_frag);
  ShaderRegistry::registerSource("prefilter_ggx_frag", shaders::prefilter_ggx_frag);
}

} // namespace blkhurst
