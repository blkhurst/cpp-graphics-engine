#include <blkhurst/materials/material.hpp>
#include <spdlog/spdlog.h>

namespace blkhurst {

Material::Material(std::shared_ptr<Program> prog)
    : program_(std::move(prog)) {
  if (program_) {
    spdlog::trace("Material constructed using Program({})", program_->id());
  } else {
    spdlog::warn("Material constructed with null Program");
  }
}

Material::~Material() {
  spdlog::trace("Material destroyed");
}

std::shared_ptr<Material> Material::create(std::shared_ptr<Program> prog) {
  return std::make_shared<Material>(prog);
}

void Material::useProgram() const {
  // Program rebuilds if needed on use()
  program_->use();
}
void Material::applyUniformsAndResources() {
  applyResources();
  applyUniforms();
}

std::shared_ptr<Program> Material::program() const {
  return program_;
}
const PipelineState& Material::pipeline() const {
  return pipeline_;
}
void Material::setDepthTest(bool enabled) {
  pipeline_.depthTest = enabled;
}
void Material::setDepthWrite(bool enabled) {
  pipeline_.depthWrite = enabled;
}
void Material::setDepthFunc(DepthFunc func) {
  pipeline_.depthFunc = func;
}
void Material::setBlend(bool enabled) {
  pipeline_.blend = enabled;
}
void Material::setCullFace(CullFace face) {
  pipeline_.cull = face;
}

void Material::setUniform(const std::string& name, int value) {
  uniforms_[name] = value;
}
void Material::setUniform(const std::string& name, float value) {
  uniforms_[name] = value;
}
void Material::setUniform(const std::string& name, const glm::vec2& value) {
  uniforms_[name] = value;
}
void Material::setUniform(const std::string& name, const glm::vec3& value) {
  uniforms_[name] = value;
}
void Material::setUniform(const std::string& name, const glm::vec4& value) {
  uniforms_[name] = value;
}
void Material::setUniform(const std::string& name, const glm::mat3& value) {
  uniforms_[name] = value;
}
void Material::setUniform(const std::string& name, const glm::mat2& value) {
  uniforms_[name] = value;
}
void Material::setUniform(const std::string& name, const glm::mat4& value) {
  uniforms_[name] = value;
}

void Material::addDefine(const std::string& def) {
  if (program_) {
    program_->addDefine(def);
  }
}
void Material::removeDefine(const std::string& def) {
  if (program_) {
    program_->removeDefine(def);
  }
}
void Material::setDefines(std::vector<std::string> defs) {
  if (program_) {
    program_->setDefines(std::move(defs));
  }
}
void Material::linkUniformBlock(const std::string& name, unsigned binding) const {
  if (program_) {
    program_->linkUniformBlock(name, binding);
  }
}
void Material::linkStorageBlock(const std::string& name, unsigned binding) const {
  if (program_) {
    program_->linkStorageBlock(name, binding);
  }
}

void Material::applyUniforms() const {
  for (const auto& [name, val] : uniforms_) {
    applyUniform(*program_, name, val);
  }
}

void Material::bindTextureUnit(const std::shared_ptr<Texture>& tex, const std::string& uniformName,
                               int slot) {
  if (tex) {
    setUniform(uniformName, slot);
    tex->bindUnit(slot);
  }
}

// Share Program, Copy Uniforms
std::shared_ptr<Material> Material::clone() const {
  auto newMaterial = std::make_shared<Material>(program_);
  newMaterial->pipeline_ = pipeline_;
  newMaterial->uniforms_ = uniforms_;
  return newMaterial;
}

void Material::applyUniform(Program& prog, const std::string& name, const UniformValue& uniform) {
  std::visit([&](const auto& value) { prog.setUniform(name, value); }, uniform);
}

} // namespace blkhurst
