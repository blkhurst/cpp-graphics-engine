#include <blkhurst/graphics/program.hpp>
#include <blkhurst/shaders/shader_preprocessor.hpp>
#include <blkhurst/util/assets.hpp>

#include <glad/gl.h>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

namespace blkhurst {

Program::Program(ProgramDesc desc)
    : desc_(std::move(desc)) {
  // spdlog in factories
}

Program::~Program() {
  if (id_ != 0U) {
    glDeleteProgram(id_);
    spdlog::trace("Program({}) deleted", id_);
  }
}

// Factory
std::shared_ptr<Program> Program::create(const ProgramDesc& desc) {
  auto program = std::make_shared<Program>(desc);
  program->sourceKind_ = SourceKind::Source;
  program->needsUpdate_ = true;
  spdlog::trace("Program(<deferred>) created (SourceKind=Source)");
  return program;
}

std::shared_ptr<Program> Program::createFromRegistry(const ProgramDesc& desc) {
  auto program = std::make_shared<Program>(desc);
  program->sourceKind_ = SourceKind::Registry;
  program->needsUpdate_ = true;
  spdlog::trace("Program(<deferred>) created (SourceKind=Registry)");
  return program;
}

std::shared_ptr<Program> Program::createFromFiles(const ProgramDesc& desc) {
  auto program = std::make_shared<Program>(desc);
  program->sourceKind_ = SourceKind::File;
  program->needsUpdate_ = true;
  spdlog::trace("Program(<deferred>) created (SourceKind=File)");
  return program;
}

void Program::use() const {
  ensureBuilt_();
  glUseProgram(id_);
}

void Program::needsUpdate() const {
  needsUpdate_ = true;
}

void Program::addDefine(const std::string& define) {
  if (std::find(desc_.defines.begin(), desc_.defines.end(), define) == desc_.defines.end()) {
    desc_.defines.push_back(define);
    needsUpdate_ = true;
  }
}

void Program::removeDefine(const std::string& define) {
  auto found = std::remove(desc_.defines.begin(), desc_.defines.end(), define);
  if (found != desc_.defines.end()) {
    desc_.defines.erase(found, desc_.defines.end());
    needsUpdate_ = true;
  }
}

void Program::setDefines(std::vector<std::string> defines) {
  std::sort(defines.begin(), defines.end());
  defines.erase(std::unique(defines.begin(), defines.end()), defines.end());
  desc_.defines = std::move(defines);
  needsUpdate_ = true;
}

// Cache response of "glGetUniformLocation" (expensive)
int Program::uniformLocation(std::string_view name) const {
  auto key = std::string(name);
  auto found = uniformCache_.find(key);
  if (found != uniformCache_.end()) {
    return found->second;
  }
  int loc = glGetUniformLocation(id_, key.c_str());
  uniformCache_.emplace(std::move(key), loc);
  return loc;
}

void Program::setUniform(std::string_view name, int value) {
  glUniform1i(uniformLocation(name), value);
}

void Program::setUniform(std::string_view name, float value) {
  glUniform1f(uniformLocation(name), value);
}

void Program::setUniform(std::string_view name, const glm::vec2& value) {
  glUniform2fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void Program::setUniform(std::string_view name, const glm::vec3& value) {
  glUniform3fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void Program::setUniform(std::string_view name, const glm::vec4& value) {
  glUniform4fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void Program::setUniform(std::string_view name, const glm::mat2& value) {
  // No need to transpose when using GLM; column-major by default
  glUniformMatrix2fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Program::setUniform(std::string_view name, const glm::mat3& value) {
  glUniformMatrix3fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Program::setUniform(std::string_view name, const glm::mat4& value) {
  glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Program::linkUniformBlock(std::string_view blockName, unsigned bindingPoint) const {
  unsigned idx = glGetUniformBlockIndex(id_, std::string(blockName).c_str());
  if (idx != GL_INVALID_INDEX) {
    glUniformBlockBinding(id_, idx, bindingPoint);
    spdlog::debug("Program({}) link UBO '{}' -> binding={}", id_, blockName, bindingPoint);
  } else {
    spdlog::warn("Program({}) UBO not found: '{}'", id_, blockName);
  }
}

void Program::linkStorageBlock(std::string_view blockName, unsigned bindingPoint) const {
  unsigned idx =
      glGetProgramResourceIndex(id_, GL_SHADER_STORAGE_BLOCK, std::string(blockName).c_str());
  if (idx != GL_INVALID_INDEX) {
    glShaderStorageBlockBinding(id_, idx, bindingPoint);
    spdlog::debug("Program({}) link SSBO '{}' -> binding={}", id_, blockName, bindingPoint);
  } else {
    spdlog::warn("Program({}) SSBO not found: '{}'", id_, blockName);
  }
}

unsigned Program::compileShader(GLenum type, std::string_view src) {
  unsigned shader = glCreateShader(type);
  const char* source = src.data();
  int len = (int)src.size();
  glShaderSource(shader, 1, &source, &len);
  glCompileShader(shader);
  checkCompile(shader, getStageString(type).c_str());
  return shader;
}

unsigned Program::linkProgram(const std::vector<GLuint>& shaders) {
  unsigned prog = glCreateProgram();
  for (auto shader : shaders) {
    glAttachShader(prog, shader);
  }
  glLinkProgram(prog);
  checkLink(prog);
  for (auto shader : shaders) {
    glDetachShader(prog, shader);
    glDeleteShader(shader);
  }
  return prog;
}

void Program::checkCompile(unsigned shader, const char* stage) {
  int compileOk = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compileOk);
  if (compileOk == 0) {
    int len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    std::string log(len, '\0');
    glGetShaderInfoLog(shader, len, nullptr, log.data());
    spdlog::error("{} Shader compile error:\n{}", stage, log);
  }
}

void Program::checkLink(unsigned prog) {
  int linkOk = 0;
  glGetProgramiv(prog, GL_LINK_STATUS, &linkOk);
  if (linkOk == 0) {
    int len = 0;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
    std::string log(len, '\0');
    glGetProgramInfoLog(prog, len, nullptr, log.data());
    spdlog::error("Program({}) link error:\n{}", prog, log);
  }
}

std::string Program::getStageString(GLenum type) {
  switch (type) {
  case GL_VERTEX_SHADER:
    return "VERTEX";
  case GL_FRAGMENT_SHADER:
    return "FRAGMENT";
  case GL_TESS_CONTROL_SHADER:
    return "TESSELLATION_CONTROL";
  case GL_TESS_EVALUATION_SHADER:
    return "TESSELLATION_EVALUATION";
  case GL_GEOMETRY_SHADER:
    return "GEOMETRY";
  case GL_COMPUTE_SHADER:
    return "COMPUTE";
  default:
    return "UNKNOWN";
  }
}

void Program::ensureBuilt_() const {
  if (!needsUpdate_) {
    return;
  }

  PreprocessOptions preprocessOptions{.defines = desc_.defines, .glslVersion = desc_.glslVersion};

  auto load = [&](std::string_view val) -> std::string {
    if (val.empty()) {
      return {};
    }
    switch (sourceKind_) {
    case SourceKind::Source:
      return ShaderPreprocessor::processSource(val, preprocessOptions);
    case SourceKind::Registry:
      return ShaderPreprocessor::processRegistry(val, preprocessOptions);
    case SourceKind::File:
      return ShaderPreprocessor::processFile(val, preprocessOptions);
    }
    return {};
  };

  const std::string vert = load(desc_.vert);
  const std::string frag = load(desc_.frag);
  const std::string tesc = load(desc_.tesc);
  const std::string tese = load(desc_.tese);

  // Compile + link
  buildFromStrings_(vert, frag, tesc, tese);

  needsUpdate_ = false;
}

// Build / Rebuild
void Program::buildFromStrings_(std::string_view vert, std::string_view frag, std::string_view tesc,
                                std::string_view tese) const {

  bool hasVert = !desc_.vert.empty();
  bool hasFrag = !desc_.frag.empty();
  bool hasTesc = !desc_.tesc.empty();
  bool hasTese = !desc_.tese.empty();
  if (!hasVert || !hasFrag) {
    spdlog::error("Program missing required stages vertex/fragment");
  }
  if ((hasTesc && !hasTese) || (!hasTesc && hasTese)) {
    spdlog::warn("Program missing required tessellation stages tesc/tese");
  }

  std::vector<GLuint> shaders;
  shaders.reserve(4);
  if (hasVert) {
    shaders.push_back(compileShader(GL_VERTEX_SHADER, vert));
  }
  if (hasFrag) {
    shaders.push_back(compileShader(GL_FRAGMENT_SHADER, frag));
  }
  if (hasTesc) {
    shaders.push_back(compileShader(GL_TESS_CONTROL_SHADER, tesc));
  }
  if (hasTese) {
    shaders.push_back(compileShader(GL_TESS_EVALUATION_SHADER, tese));
  }

  const GLuint newId = linkProgram(shaders);

  if (id_ != 0U) {
    glDeleteProgram(id_);
  }
  id_ = newId;
  uniformCache_.clear();

  spdlog::trace("Program({}) built (V:{} F:{} TC:{} TE:{})", id_, !vert.empty(), !frag.empty(),
                !tesc.empty(), !tese.empty());
}

} // namespace blkhurst
