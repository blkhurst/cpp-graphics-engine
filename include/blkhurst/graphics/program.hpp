#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace blkhurst {

struct PreprocessOptions {
  std::vector<std::string> macros;
  std::string glslVersion = "450 core";
  bool insertHeader = true;
};

class Program {
public:
  Program() = default;
  Program(std::string_view vertSrc, std::string_view fragSrc);
  static Program fromFiles(std::string_view vertPath, std::string_view fragPath,
                           const PreprocessOptions& opts = {});

  ~Program();

  Program(const Program&) = delete;
  Program(Program&&) = delete;
  Program& operator=(const Program&) = delete;
  Program& operator=(Program&&) = delete;

  void use() const;

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
  static unsigned linkProgram(std::initializer_list<unsigned> shaders);
  static void checkCompile(unsigned shader, const char* stage);
  static void checkLink(unsigned prog);

  static std::string getStageString(unsigned type);
  int uniformLocation(std::string_view name) const;

  static std::string preprocess(std::string_view source, const PreprocessOptions& opts,
                                std::string_view currentDir = {});
  static std::string preprocessFile(std::string_view path, const PreprocessOptions& opts);

private:
  unsigned id_ = 0;
  mutable std::unordered_map<std::string, int> uniformCache_;
};

} // namespace blkhurst
