#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace blkhurst {

struct ProgramDesc {
  std::string vert;
  std::string frag;
  std::string tesc;
  std::string tese;
  std::string comp;
  std::vector<std::string> defines;
  std::string glslVersion = "450 core";
};

enum class SourceKind { Source, Registry, File };

class Program {
public:
  Program(ProgramDesc desc);
  virtual ~Program();

  Program(const Program&) = delete;
  Program(Program&&) = delete;
  Program& operator=(const Program&) = delete;
  Program& operator=(Program&&) = delete;

  static std::shared_ptr<Program> create(const ProgramDesc& desc);
  static std::shared_ptr<Program> createFromRegistry(const ProgramDesc& desc);
  static std::shared_ptr<Program> createFromFiles(const ProgramDesc& desc);

  void use() const;

  void needsUpdate() const;
  void addDefine(const std::string& define);
  void removeDefine(const std::string& define);
  void setDefines(std::vector<std::string> defines);

  void setUniform(std::string_view name, int value);
  void setUniform(std::string_view name, float value);
  void setUniform(std::string_view name, const glm::vec2& value);
  void setUniform(std::string_view name, const glm::vec3& value);
  void setUniform(std::string_view name, const glm::vec4& value);
  void setUniform(std::string_view name, const glm::mat2& value);
  void setUniform(std::string_view name, const glm::mat3& value);
  void setUniform(std::string_view name, const glm::mat4& value);
  void setSampler(std::string_view name, unsigned textureID, int unit);

  // UBO / SSBO block binding points
  void linkUniformBlock(std::string_view blockName, unsigned bindingPoint) const;
  void linkStorageBlock(std::string_view blockName, unsigned bindingPoint) const;

  unsigned id() const {
    return id_;
  }

protected:
  Program(unsigned alreadyLinked)
      : id_(alreadyLinked) {
  }

  static unsigned compileShader(unsigned type, std::string_view src);
  static unsigned linkProgram(const std::vector<unsigned>& shaders);
  static void checkCompile(unsigned shader, const char* stage);
  static void checkLink(unsigned prog);

  static std::string getStageString(unsigned type);
  int uniformLocation(std::string_view name) const;

private:
  mutable unsigned id_ = 0;
  mutable std::unordered_map<std::string, int> uniformCache_;

  ProgramDesc desc_;
  SourceKind sourceKind_ = SourceKind::Source;

  mutable bool needsUpdate_ = true;
  void ensureBuilt_() const;
  void buildFromStrings_(std::string_view vert, std::string_view frag, std::string_view tesc,
                         std::string_view tese) const;
};

} // namespace blkhurst
