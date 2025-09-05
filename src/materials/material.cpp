#include <blkhurst/materials/material.hpp>
#include <spdlog/spdlog.h>

namespace blkhurst {

Material::Material(std::shared_ptr<Program> prog)
    : program(std::move(prog)) {
  spdlog::trace("Material constructed using Program({})", program->id());
}

Material::~Material() {
  spdlog::trace("Material destroyed");
}

std::shared_ptr<Material> Material::create(std::shared_ptr<Program> prog) {
  return std::make_shared<Material>(prog);
}

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
