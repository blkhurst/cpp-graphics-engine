#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace blkhurst {

/**
 * Asset path resolver.
 * Root order:
 *   1) Environment variable paths (env_var, default "BLKHURST_ASSETS")
 *   2) Install root (if set)
 *   3) Current working directory
 *   4) Executable directory
 *   5) Each absolute search path
 * For each root, check:
 *   1) <root>/<rel>
 *   2) <root>/<rel-prefix>/<rel>
 *
 * Notes:
 *  - If the input is absolute and exists, it's returned immediately.
 *  - Environment variable may contain multiple paths separated by the OS path-list delimiter
 *    (':' on POSIX, ';' on Windows).
 */
class Assets {
public:
  Assets() = default;

  void setInstallRoot(const std::string& root);
  void setSearchPaths(const std::vector<std::string>& paths);
  void addSearchPath(const std::string& path);
  void setEnvVarName(std::string name);

  [[nodiscard]] const std::filesystem::path& installRoot() const;
  [[nodiscard]] const std::vector<std::filesystem::path>& searchPaths() const;
  [[nodiscard]] const std::string& envVarName() const;

  [[nodiscard]] std::optional<std::string> find(std::string_view file) const;
  [[nodiscard]] std::string readText(std::string_view pathLike) const;

  static std::optional<std::filesystem::path> exeDir();
  static std::filesystem::path cwd();

private:
  std::filesystem::path installRoot_;
  std::vector<std::filesystem::path> searchPaths_;
  std::string envVarName_ = "BLKHURST_ASSETS";

  [[nodiscard]] std::vector<std::filesystem::path> buildSearchOrder() const;
  [[nodiscard]] std::vector<std::filesystem::path> parseEnvPaths() const;
  static std::filesystem::path weaklyCanonicalOrOriginal(const std::filesystem::path& path);
};

} // namespace blkhurst

// Assets Singleton
namespace blkhurst::assets {

inline Assets& ctx() {
  static Assets instance;
  return instance;
}

inline void setInstallRoot(const std::string& root) {
  ctx().setInstallRoot(root);
}
inline void setSearchPaths(const std::vector<std::string>& paths) {
  ctx().setSearchPaths(paths);
}
inline void addSearchPath(const std::string& path) {
  ctx().addSearchPath(path);
}
inline void setEnvVarName(const std::string& name) {
  ctx().setEnvVarName(name);
}
[[nodiscard]] inline std::optional<std::string> find(std::string_view file) {
  return ctx().find(file);
}
[[nodiscard]] inline std::string readText(std::string_view pathLike) {
  return ctx().readText(pathLike);
}
[[nodiscard]] inline std::optional<std::filesystem::path> exeDir() {
  return Assets::exeDir();
}
[[nodiscard]] inline std::filesystem::path cwd() {
  return Assets::cwd();
}

} // namespace blkhurst::assets
