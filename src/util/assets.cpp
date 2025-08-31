#include <blkhurst/util/assets.hpp>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <spdlog/spdlog.h>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace fs = std::filesystem;
namespace blkhurst::assets {

namespace {
std::mutex& mtx() {
  static std::mutex mutexInstance;
  return mutexInstance;
}
std::vector<fs::path>& user_paths() {
  static std::vector<fs::path> userPathsVec;
  return userPathsVec;
}
fs::path& install_root() {
  static fs::path installRootPath;
  return installRootPath;
}
bool& inited() {
  static bool initializedFlag = false;
  return initializedFlag;
}

char path_delim() {
#ifdef _WIN32
  return ';';
#else
  return ':';
#endif
}

std::optional<fs::path> exe_dir() {
#ifdef _WIN32
  char buf[MAX_PATH];
  DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
  if (len == 0 || len == MAX_PATH)
    return std::nullopt;
  return fs::path(buf).parent_path();
#elif defined(__APPLE__)
  uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size);
  std::string buf(size, '\0');
  if (_NSGetExecutablePath(buf.data(), &size) != 0)
    return std::nullopt;
  return fs::weakly_canonical(fs::path(buf).parent_path());
#elif defined(__linux__)
  std::error_code errorCode;
  auto symlinkPath = fs::read_symlink("/proc/self/exe", errorCode);
  if (errorCode) {
    return std::nullopt;
  }
  return fs::weakly_canonical(symlinkPath.parent_path());
#else
  return std::nullopt;
#endif
}

void init_defaults_if_needed() {
  if (inited()) {
    return;
  }
  std::scoped_lock lock(mtx());
  if (inited()) {
    return;
  }

  // Env var: BLKHURST_ASSETS=a:b:c (or ; on Windows)
  if (const char* env = std::getenv("BLKHURST_ASSETS")) {
    std::string envStr(env);
    std::string acc;
    for (char chr : envStr) {
      if (chr == path_delim()) {
        if (!acc.empty()) {
          user_paths().emplace_back(acc), acc.clear();
        }
      } else {
        acc.push_back(chr);
      }
    }
    if (!acc.empty()) {
      user_paths().emplace_back(acc);
    }
  }

  // Default: current working dir
  user_paths().push_back(fs::current_path());

  // Default: executable dir (best-effort)
  if (auto exeDir = exe_dir()) {
    user_paths().push_back(*exeDir);
  }

  // Optional install root (set via set_install_root)
  if (!install_root().empty()) {
    user_paths().push_back(install_root());
  }

  inited() = true;
}

bool exists_file(const fs::path& path) {
  std::error_code errorCode;
  auto status = fs::status(path, errorCode);
  return !errorCode && fs::exists(status) && fs::is_regular_file(status);
}

} // namespace

void add_search_path(const fs::path& path) {
  init_defaults_if_needed();
  std::scoped_lock lock(mtx());

  auto push_unique = [](const fs::path& path) {
    const fs::path canon = fs::weakly_canonical(path);
    auto& roots = user_paths();
    if (std::find(roots.begin(), roots.end(), canon) == roots.end()) {
      roots.push_back(canon);
      spdlog::trace("Assets: added search path '{}'", canon.string());
    }
  };

  if (path.is_absolute()) {
    push_unique(path);
  } else {
    push_unique(fs::current_path() / path);
    if (auto exeDir = exe_dir()) {
      push_unique(*exeDir / path);
    }
  }
}

void clear_search_paths() {
  std::scoped_lock lock(mtx());
  user_paths().clear();
  inited() = false; // force re-init next time (re-reads env, cwd, exe dir)
  spdlog::trace("Assets: cleared search paths");
}

void set_install_root(const fs::path& path) {
  std::scoped_lock lock(mtx());
  install_root() = path;
  spdlog::trace("Assets: set install root '{}'", path.string());
}

std::optional<fs::path> find(std::string_view relOrAbs) {
  init_defaults_if_needed();
  const fs::path inPath(relOrAbs);

  // Absolute?
  if (inPath.is_absolute() && exists_file(inPath)) {
    spdlog::trace("Assets: found absolute '{}'", inPath.string());
    return fs::weakly_canonical(inPath);
  }

  // Try user paths + defaults
  std::scoped_lock lock(mtx());
  for (const auto& root : user_paths()) {
    fs::path cand = root / inPath;
    if (exists_file(cand)) {
      spdlog::trace("Assets: found '{}' in '{}'", inPath.string(), root.string());
      return fs::weakly_canonical(cand);
    }
  }

  spdlog::warn("Assets: not found '{}'", inPath.string());
  return std::nullopt;
}

std::string read_text(std::string_view pathLike) {
  auto foundPath = find(pathLike);
  if (!foundPath) {
    return {};
  }
  std::ifstream file(*foundPath, std::ios::in | std::ios::binary);
  if (!file) {
    spdlog::error("Assets: read failed (open error): '{}'", foundPath->string());
    return {};
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

} // namespace blkhurst::assets
