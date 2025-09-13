#include <blkhurst/shaders/shader_preprocessor.hpp>

#include <blkhurst/shaders/shader_registry.hpp>
#include <blkhurst/util/assets.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>

namespace blkhurst {

enum class IncludeMode { Mixed /* file + registry */, RegistryOnly };

// Seen keys prefixes to avoid collisions between sources
static constexpr std::string_view kSeenRegPrefix = "registry://";
static constexpr std::string_view kSeenFilePrefix = "file://";

// Helpers
namespace {
inline bool isIncludeDirective(const std::string& line, std::string& quotedPath) {
  if (line.rfind("#include", 0) != 0) {
    return false;
  }
  const auto first = line.find('"');
  if (first == std::string::npos) {
    return false;
  }
  const auto last = line.find('"', first + 1);
  if (last == std::string::npos || last <= first + 1) {
    return false;
  }
  quotedPath.assign(line, first + 1, last - first - 1);
  return true;
}

inline void writeHeaderIfFirst(std::ostringstream& out, const PreprocessOptions& opts,
                               bool isFirstChunk) {
  if (!isFirstChunk) {
    return;
  }
  if (!opts.glslVersion.empty()) {
    out << "#version " << opts.glslVersion << "\n";
  }
  for (const auto& define : opts.defines) {
    out << "#define " << define << "\n";
  }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
inline std::string normaliseJoin(std::string_view currentDir, std::string_view relative) {
  // Prevents duplicates via different relative paths
  const std::filesystem::path base{std::string(currentDir)};
  const std::filesystem::path joined =
      base.empty() ? std::filesystem::path(std::string(relative)) : (base / std::string(relative));
  return joined.lexically_normal().string();
}

inline bool markSeen(std::unordered_set<std::string>& seen, std::string_view key) {
  return seen.insert(std::string(key)).second;
}

} // namespace

// Forward declare
static std::string preprocessSourceInternal(std::string_view source, const PreprocessOptions& opts,
                                            std::string_view currentDir,
                                            std::unordered_set<std::string>& seen,
                                            IncludeMode mode);

static std::string preprocessFileInternal(std::string_view path, const PreprocessOptions& opts,
                                          std::unordered_set<std::string>& seen, IncludeMode mode) {
  auto src = assets::readText(path);
  const std::string curDir = std::filesystem::path(std::string(path)).parent_path().string();
  return preprocessSourceInternal(src, opts, curDir, seen, mode);
}

// Core
static std::string preprocessSourceInternal(std::string_view source, const PreprocessOptions& opts,
                                            std::string_view currentDir,
                                            std::unordered_set<std::string>& seen,
                                            IncludeMode mode) {
  std::istringstream sourceIn{std::string(source)};
  std::ostringstream sourceOut;

  writeHeaderIfFirst(sourceOut, opts, seen.empty());

  std::string line;
  while (std::getline(sourceIn, line)) {
    std::string includeName;
    if (!isIncludeDirective(line, includeName)) {
      sourceOut << line << '\n';
      continue;
    }

    // 1) Try ShaderRegistry
    if (ShaderRegistry::has(includeName)) {
      const std::string seenKey = std::string(kSeenRegPrefix) + includeName;
      if (!markSeen(seen, seenKey)) {
        spdlog::warn("Shader include suppressed (already included once from registry): {}",
                     includeName);
        continue;
      }

      auto regSrc = ShaderRegistry::find(includeName);
      if (!regSrc) {
        spdlog::warn("Shader include failed: {}", includeName);
        continue;
      }

      sourceOut << preprocessSourceInternal(*regSrc, opts, /*currentDir*/ "", seen, mode) << '\n';
      continue;
    }

    // 2) If RegistryOnly, do not attempt file resolution
    if (mode == IncludeMode::RegistryOnly) {
      spdlog::warn("Shader include '{}' not found in registry (RegistryOnly); skipping.",
                   includeName);
      continue;
    }

    // 3) Resolve as file path (Mixed mode)
    const std::string fullPath = normaliseJoin(currentDir, includeName);
    const std::string seenKey = std::string(kSeenFilePrefix) + fullPath;
    if (!markSeen(seen, seenKey)) {
      spdlog::warn("Shader include suppressed (already included once): {}", fullPath);
      continue;
    }

    sourceOut << preprocessFileInternal(fullPath, opts, seen, mode) << '\n';
  }

  return sourceOut.str();
}

// Public
std::string ShaderPreprocessor::processFile(std::string_view path, const PreprocessOptions& opts) {
  std::unordered_set<std::string> seen;
  spdlog::trace("ShaderPreprocessor processing file({})", path);
  return preprocessFileInternal(path, opts, seen, IncludeMode::Mixed);
}

std::string ShaderPreprocessor::processRegistry(std::string_view name,
                                                const PreprocessOptions& opts) {
  std::unordered_set<std::string> seen;
  spdlog::trace("ShaderPreprocessor processing registry({})", name);

  auto src = ShaderRegistry::find(name);
  if (!src) {
    spdlog::error("ShaderPreprocessor '{}' not found in registry.", name);
    return {};
  }
  return preprocessSourceInternal(*src, opts, /*currentDir*/ "", seen, IncludeMode::RegistryOnly);
}

std::string ShaderPreprocessor::processSource(std::string_view source,
                                              const PreprocessOptions& opts) {
  std::unordered_set<std::string> seen;
  spdlog::trace("ShaderPreprocessor processing source...");
  return preprocessSourceInternal(source, opts, /*currentDir*/ "", seen, IncludeMode::RegistryOnly);
}

} // namespace blkhurst
