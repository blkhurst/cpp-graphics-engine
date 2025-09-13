#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace blkhurst {

class ShaderRegistry {
public:
  ShaderRegistry() = delete;
  static ShaderRegistry& instance();

  static void registerSource(std::string name, std::string source);

  [[nodiscard]] static bool has(std::string_view name);
  [[nodiscard]] static std::optional<std::string> find(std::string_view name);

  static void registerBuiltinShaders();

private:
  static std::unordered_map<std::string, std::string>& map_();
};

} // namespace blkhurst
