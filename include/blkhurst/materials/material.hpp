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

  void useProgram() const;
  void applyUniformsAndResources();

  [[nodiscard]] std::shared_ptr<Program> program() const;
  [[nodiscard]] const PipelineState& pipeline() const;
  void setDepthTest(bool enabled);
  void setDepthWrite(bool enabled);
  void setDepthFunc(DepthFunc func);
  void setBlend(bool enabled);
  void setCullFace(CullFace face);

  void setUniform(const std::string& name, int value);
  void setUniform(const std::string& name, float value);
  void setUniform(const std::string& name, const glm::vec2& value);
  void setUniform(const std::string& name, const glm::vec3& value);
  void setUniform(const std::string& name, const glm::vec4& value);
  void setUniform(const std::string& name, const glm::mat3& value);
  void setUniform(const std::string& name, const glm::mat2& value);
  void setUniform(const std::string& name, const glm::mat4& value);

  void addDefine(const std::string& def);
  void removeDefine(const std::string& def);
  void setDefines(std::vector<std::string> defs);
  void linkUniformBlock(const std::string& name, unsigned binding) const;
  void linkStorageBlock(const std::string& name, unsigned binding) const;

  virtual std::shared_ptr<Material> clone() const;
  template <class T> std::shared_ptr<T> cloneAs() const {
    return std::dynamic_pointer_cast<T>(this->clone());
  }

protected:
  void applyUniforms() const;
  virtual void applyResources() {};

private:
  PipelineState pipeline_;
  std::shared_ptr<Program> program_;
  std::unordered_map<std::string, UniformValue> uniforms_;

  static void applyUniform(Program& prog, const std::string& name, const UniformValue& uniform);
};

} // namespace blkhurst
