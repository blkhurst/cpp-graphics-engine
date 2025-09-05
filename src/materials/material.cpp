#include <blkhurst/materials/material.hpp>
#include <spdlog/spdlog.h>

namespace blkhurst {

void Material::apply() const {
  if (!program) {
    spdlog::error("Material::apply called with null Program");
    return;
  }
  program->use();

  for (const auto& [name, val] : uniforms) {
    applyUniform(*program, name, val);
  }
}

void Material::applyUniform(Program& prog, const std::string& name, const UniformValue& uniform) {
  std::visit([&](const auto& value) { prog.setUniform(name, value); }, uniform);
}

} // namespace blkhurst
