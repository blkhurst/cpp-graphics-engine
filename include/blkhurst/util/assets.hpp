#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace blkhurst::assets {

void set_install_root(const std::filesystem::path& path);
void add_search_path(const std::filesystem::path& path);
void clear_search_paths();

// Find a file; tries absolute path, environment, cwd, exe-dir, install_root.
std::optional<std::filesystem::path> find(std::string_view relOrAbs);

// Uses find internally
std::string read_text(std::string_view pathLike);

} // namespace blkhurst::assets
