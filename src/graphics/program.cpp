// NOLINTBEGIN(readability-identifier-length)
#include "graphics/program.hpp"
#include <fstream>
#include <glad/gl.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>

namespace blkhurst {

struct Shaders {
  std::string vertPath;
  std::string fragPath;
  std::string compPath;
  std::string tessEvalPath;
};

Program::Program(std::string_view vertSrc, std::string_view fragSrc)
    : id_(linkProgram(
          {compileShader(GL_VERTEX_SHADER, vertSrc), compileShader(GL_FRAGMENT_SHADER, fragSrc)})) {
  spdlog::trace("Program({}) created (VS+FS)", id_);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Program Program::fromFiles(std::string_view vertPath, std::string_view fragPath,
                           const PreprocessOptions& opts) {
  spdlog::trace("Program preprocessing VS='{}' FS='{}'", vertPath, fragPath);
  auto vs = preprocessFile(vertPath, opts);
  auto fs = preprocessFile(fragPath, opts);
  return {vs, fs};
}

Program::~Program() {
  if (id_ != 0U) {
    glDeleteProgram(id_);
    spdlog::trace("Program({}) deleted", id_);
  }
}

void Program::use() const {
  glUseProgram(id_);
}

// Cache response of "glGetUniformLocation" (expensive)
int Program::uniformLocation(std::string_view name) const {
  auto key = std::string(name);
  auto it = uniformCache_.find(key);
  if (it != uniformCache_.end()) {
    return it->second;
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

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Program::setSampler(std::string_view name, GLuint textureID, int unit) {
  glUniform1i(uniformLocation(name), unit); // Set the sampler uniform
  glActiveTexture(GL_TEXTURE0 + unit);      // Activate the specific texture unit
  glBindTexture(GL_TEXTURE_2D, textureID);  // Bind the texture to that unit
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
  const char* s = src.data();
  int len = (int)src.size();
  glShaderSource(shader, 1, &s, &len);
  glCompileShader(shader);
  checkCompile(shader, getStageString(type).c_str());
  return shader;
}

unsigned Program::linkProgram(std::initializer_list<unsigned> shaders) {
  unsigned prog = glCreateProgram();
  for (auto sh : shaders) {
    glAttachShader(prog, sh);
  }
  glLinkProgram(prog);
  checkLink(prog);
  for (auto sh : shaders) {
    glDetachShader(prog, sh);
    glDeleteShader(sh);
  }
  return prog;
}

void Program::checkCompile(unsigned shader, const char* stage) {
  int ok = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (ok == 0) {
    int len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    std::string log(len, '\0');
    glGetShaderInfoLog(shader, len, nullptr, log.data());
    spdlog::error("{} Shader compile error:\n{}", stage, log);
  }
}

void Program::checkLink(unsigned prog) {
  int ok = 0;
  glGetProgramiv(prog, GL_LINK_STATUS, &ok);
  if (ok == 0) {
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

// ============== Read files + Preprocess ==============
std::string Program::readTextFile(std::string_view path) {
  std::ifstream f(std::string(path), std::ios::in | std::ios::binary);
  if (!f) {
    spdlog::error("Program read failed: {}", path);
  }
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

// TODO: ...
static std::string preprocessImpl(std::string_view source, const PreprocessOptions& opts,
                                  std::string_view currentDir,
                                  std::unordered_set<std::string>& seen,
                                  const std::function<std::string(std::string_view)>& reader);

static std::string preprocessFileImpl(std::string_view path, const PreprocessOptions& opts,
                                      std::unordered_set<std::string>& seen,
                                      const std::function<std::string(std::string_view)>& reader) {
  auto src = reader(path);
  auto pos = std::string(path).find_last_of("/\\");
  std::string curDir =
      (pos == std::string::npos) ? std::string{} : std::string(path).substr(0, pos);
  return preprocessImpl(src, opts, curDir, seen, reader);
}

std::string Program::preprocess(std::string_view source, const PreprocessOptions& opts,
                                std::string_view currentDir) {
  std::unordered_set<std::string> seen;
  auto reader = [](std::string_view p) { return Program::readTextFile(p); };
  return preprocessImpl(source, opts, currentDir, seen, reader);
}

std::string Program::preprocessFile(std::string_view path, const PreprocessOptions& opts) {
  std::unordered_set<std::string> seen;
  auto reader = [](std::string_view p) { return Program::readTextFile(p); };
  return preprocessFileImpl(path, opts, seen, reader);
}

namespace {

inline bool parseIncludeQuoted(const std::string& line, std::string& relPath) {
  if (line.rfind("#include", 0) != 0) {
    return false;
  }
  auto first = line.find('"');
  if (first == std::string::npos) {
    return false;
  }
  auto last = line.find('"', first + 1);
  if (last == std::string::npos || last <= first + 1) {
    return false;
  }
  relPath.assign(line, first + 1, last - first - 1);

  return true;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
inline std::string joinPath(std::string_view base, std::string_view rel) {
  if (base.empty()) {
    return std::string(rel);
  }
  std::string out(base);
  if (out.back() != '/' && out.back() != '\\') {
    out.push_back('/');
  }
  out.append(rel);
  return out;
}

inline void emitHeaderOnce(std::ostringstream& out, const blkhurst::PreprocessOptions& opts,
                           bool isFirst) {
  if (!opts.insertHeader || !isFirst) {
    return;
  }
  if (!opts.glslVersion.empty()) {
    out << "#version " << opts.glslVersion << "\n";
  }
  for (const auto& m : opts.macros) {
    out << "#define " << m << "\n";
  }
}

} // namespace
static std::string preprocessImpl(std::string_view source, const blkhurst::PreprocessOptions& opts,
                                  std::string_view currentDir,
                                  std::unordered_set<std::string>& seen,
                                  const std::function<std::string(std::string_view)>& reader) {
  std::istringstream in{std::string(source)};
  std::ostringstream out;

  emitHeaderOnce(out, opts, seen.empty());

  std::string line;
  while (std::getline(in, line)) {
    std::string rel;
    if (!parseIncludeQuoted(line, rel)) {
      out << line << '\n';
      continue;
    }

    const std::string base = opts.includeRoot.empty() ? std::string(currentDir) : opts.includeRoot;
    const std::string full = joinPath(base, rel);

    if (!seen.insert(full).second) {
      spdlog::warn("Shader include suppressed (already included once): {}", full);
      continue;
    }

    out << preprocessFileImpl(full, opts, seen, reader) << '\n';
  }

  return out.str();
}

} // namespace blkhurst

// NOLINTEND(readability-identifier-length)
