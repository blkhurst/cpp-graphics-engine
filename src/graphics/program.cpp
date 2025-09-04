// NOLINTBEGIN(readability-identifier-length)
#include <blkhurst/graphics/program.hpp>
#include <blkhurst/util/assets.hpp>

#include <filesystem>
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
static std::string preprocessImpl(std::string_view source, const PreprocessOptions& opts,
                                  std::string_view currentDir,
                                  std::unordered_set<std::string>& seen);

static std::string preprocessFileImpl(std::string_view path, const PreprocessOptions& opts,
                                      std::unordered_set<std::string>& seen) {
  auto src = assets::read_text(path);
  const std::string curDir = std::filesystem::path(std::string(path)).parent_path().string();
  return preprocessImpl(src, opts, curDir, seen);
}

std::string Program::preprocess(std::string_view source, const PreprocessOptions& opts,
                                std::string_view currentDir) {
  std::unordered_set<std::string> seen;
  return preprocessImpl(source, opts, currentDir, seen);
}

std::string Program::preprocessFile(std::string_view path, const PreprocessOptions& opts) {
  std::unordered_set<std::string> seen;
  return preprocessFileImpl(path, opts, seen);
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

inline void emitHeaderOnce(std::ostringstream& out, const PreprocessOptions& opts, bool isFirst) {
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
static std::string preprocessImpl(std::string_view source, const PreprocessOptions& opts,
                                  std::string_view currentDir,
                                  std::unordered_set<std::string>& seen) {
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

    const std::filesystem::path basePath = std::string(currentDir);
    const std::filesystem::path fullPath =
        basePath.empty() ? std::filesystem::path(rel) : (basePath / std::string(rel));
    // Normalise the joined include to prevent duplicates via different relative paths
    const std::string full = fullPath.lexically_normal().string();

    if (!seen.insert(full).second) {
      spdlog::warn("Shader include suppressed (already included once): {}", full);
      continue;
    }

    out << preprocessFileImpl(full, opts, seen) << '\n';
  }

  return out.str();
}

} // namespace blkhurst

// NOLINTEND(readability-identifier-length)
