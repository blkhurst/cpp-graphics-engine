#include <blkhurst/shaders/shader_registry.hpp>
#include <spdlog/spdlog.h>

#include <blkhurst/shaders/builtin/basic.glsl.hpp>

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
  // BasicMaterial
  ShaderRegistry::registerSource("basicVert", shaders::kBasicVert);
  ShaderRegistry::registerSource("basicFrag", shaders::kBasicFrag);
}

} // namespace blkhurst
