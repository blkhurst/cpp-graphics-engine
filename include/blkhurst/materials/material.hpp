#pragma once

#include <blkhurst/graphics/program.hpp>
#include <blkhurst/materials/pipeline_state.hpp>

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

namespace blkhurst {

using UniformValue =
    std::variant<int, float, glm::vec2, glm::vec3, glm::vec4, glm::mat2, glm::mat3, glm::mat4>;

class Material {
public:
  Material(std::shared_ptr<Program> prog);
  virtual ~Material();

  Material(const Material&) = delete;
  Material& operator=(const Material&) = delete;
  Material(Material&&) = delete;
  Material& operator=(Material&&) = delete;

  static std::shared_ptr<Material> create(std::shared_ptr<Program> prog);

  std::shared_ptr<Program> program;
  PipelineState pipeline;

  std::unordered_map<std::string, UniformValue> uniforms;

  void setUniform(const std::string& name, int value) {
    uniforms[name] = value;
  }
  void setUniform(const std::string& name, float value) {
    uniforms[name] = value;
  }
  void setUniform(const std::string& name, const glm::vec2& value) {
    uniforms[name] = value;
  }
  void setUniform(const std::string& name, const glm::vec3& value) {
    uniforms[name] = value;
  }
  void setUniform(const std::string& name, const glm::vec4& value) {
    uniforms[name] = value;
  }
  void setUniform(const std::string& name, const glm::mat3& value) {
    uniforms[name] = value;
  }
  void setUniform(const std::string& name, const glm::mat2& value) {
    uniforms[name] = value;
  }
  void setUniform(const std::string& name, const glm::mat4& value) {
    uniforms[name] = value;
  }

  void applyUniforms() const;

private:
  static void applyUniform(Program& prog, const std::string& name, const UniformValue& uniform);
};

} // namespace blkhurst
