#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace blkhurst {

struct PreprocessOptions {
  std::vector<std::string> defines;
  std::string glslVersion = "450 core";
};

class ShaderPreprocessor {
public:
  // Load + preprocess source (Includes check registry)
  static std::string processSource(std::string_view source, const PreprocessOptions& opts = {});
  // Load + preprocess by registry name (Includes check registry)
  static std::string processRegistry(std::string_view name, const PreprocessOptions& opts = {});
  // Load + preprocess by file path (Includes check registry + path/relative/registry)
  static std::string processFile(std::string_view path, const PreprocessOptions& opts = {});
};

} // namespace blkhurst
