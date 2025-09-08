#include <blkhurst/util/assets.hpp>
#include <filesystem>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>

#ifdef _WIN32
#include <windows.h> // GetModuleFileNameA
#elif defined(__APPLE__)
#include <mach-o/dyld.h> // _NSGetExecutablePath
#endif

namespace fs = std::filesystem;

namespace {

#ifdef _WIN32
constexpr char kPathListDelimiter = ';';
#else
constexpr char kPathListDelimiter = ':';
#endif

bool exists_any(const fs::path& path) {
  std::error_code errorCode;
  return fs::exists(path, errorCode);
}

bool is_file(const fs::path& path) {
  std::error_code errorCode;
  return fs::is_regular_file(path, errorCode);
}

fs::path join_clean(const fs::path& base, const fs::path& rel) {
  // Avoid double separators and handle "."/".."
  std::error_code errorCode;
  auto combined = fs::weakly_canonical(base / rel, errorCode);
  if (!errorCode) {
    return combined;
  }
  return base / rel;
}

} // namespace

namespace blkhurst {

// Configuration
void Assets::setInstallRoot(const std::string& root) {
  installRoot_ = weaklyCanonicalOrOriginal(root);
  spdlog::trace("Assets setInstallRoot {}", installRoot_.string());
}

void Assets::setSearchPaths(const std::vector<std::string>& paths) {
  searchPaths_.clear();
  for (const auto& path : paths) {
    addSearchPath(path);
  }
}

void Assets::addSearchPath(const std::string& path) {
  if (path.empty()) {
    spdlog::warn("Assets addSearchPath ignored empty path");
    return;
  }

  // Keep relativeness; DO NOT canonicalize here; but normalise ".." and "."
  fs::path normalisedPath = fs::path(path).lexically_normal();
  const auto found = std::find(searchPaths_.begin(), searchPaths_.end(), normalisedPath);
  if (found == searchPaths_.end()) {
    searchPaths_.push_back(normalisedPath);
    spdlog::trace("Assets addSearchPath {}", normalisedPath.string());
  } else {
    spdlog::trace("Assets addSearchPath duplicate ignored {}", normalisedPath.string());
  }
}

void Assets::setEnvVarName(std::string name) {
  envVarName_ = std::move(name);
  spdlog::trace("Assets setEnvVarName {}", envVarName_);
}

const fs::path& Assets::installRoot() const {
  return installRoot_;
}

const std::vector<fs::path>& Assets::searchPaths() const {
  return searchPaths_;
}

const std::string& Assets::envVarName() const {
  return envVarName_;
}

std::optional<std::string> Assets::find(std::string_view file) const {
  if (file.empty()) {
    spdlog::warn("Assets find called with empty path");
    return std::nullopt;
  }

  const fs::path inPath{file};

  // Absolute
  if (inPath.is_absolute()) {
    if (exists_any(inPath)) {
      auto out = weaklyCanonicalOrOriginal(inPath);
      spdlog::debug("Assets find absolute OK: {}", out.string());
      return out.string();
    }
    spdlog::warn("Assets absolute not found: {}", inPath.string());
    return std::nullopt;
  }

  const auto roots = buildSearchOrder();

  // Collect relative prefixes
  std::vector<fs::path> relPrefixes;
  relPrefixes.reserve(searchPaths_.size());
  for (const auto& path : searchPaths_) {
    if (!path.is_absolute()) {
      relPrefixes.push_back(path);
    }
  }

  spdlog::trace("Assets find searching {} roots, {} rel-prefixes for '{}'", roots.size(),
                relPrefixes.size(), std::string(file));

  auto try_one = [&](const fs::path& candidate) -> std::optional<fs::path> {
    if (exists_any(candidate)) {
      spdlog::debug("Assets found: {}", candidate.string());
      return candidate.string();
    }
    spdlog::trace("Assets miss: {}", candidate.string());
    return std::nullopt;
  };

  // 1) <root>/<rel>
  for (const auto& root : roots) {
    if (auto hit = try_one(join_clean(root, inPath))) {
      return hit;
    }
  }

  // 2) <root>/<rel-prefix>/<rel>
  for (const auto& root : roots) {
    for (const auto& prefix : relPrefixes) {
      if (auto hit = try_one(join_clean(join_clean(root, prefix), inPath))) {
        return hit;
      }
    }
  }

  spdlog::warn("Assets not found '{}'", std::string(file));
  return std::nullopt;
}

std::string Assets::readText(std::string_view pathLike) const {
  const auto found = find(pathLike);
  if (!found) {
    return {};
  }
  std::ifstream file(*found, std::ios::in | std::ios::binary);
  if (!file) {
    spdlog::error("Assets readText failed to open '{}'", found->c_str());
    return {};
  }

  std::ostringstream buffer;
  buffer << file.rdbuf();
  const std::string out = buffer.str();
  spdlog::trace("Assets readText {} bytes from {}", out.size(), found->c_str());
  return out;
}

std::optional<fs::path> Assets::exeDir() {
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

fs::path Assets::cwd() {
  std::error_code errorCode;
  auto path = fs::current_path(errorCode);
  if (errorCode) {
    return {};
  }
  return path;
}

// Private
std::vector<fs::path> Assets::buildSearchOrder() const {
  std::vector<fs::path> order;

  // 1) Env-var roots
  auto envPaths = parseEnvPaths();
  order.insert(order.end(), envPaths.begin(), envPaths.end());

  // 2) Install root
  if (!installRoot_.empty()) {
    order.push_back(installRoot_);
  }

  // 3) CWD
  if (auto currentDir = cwd(); !currentDir.empty()) {
    order.push_back(currentDir);
  }

  // 4) Executable dir
  if (auto exDir = exeDir()) {
    order.push_back(*exDir);
  }

  // 5) Absolute configured search paths behave like independent roots
  for (const auto& path : searchPaths_) {
    if (path.is_absolute()) {
      order.push_back(path);
    }
  }

  // Dedup + canonicalise
  std::vector<fs::path> deduped;
  deduped.reserve(order.size());
  for (const auto& path : order) {
    if (path.empty()) {
      continue;
    }
    const auto canon = weaklyCanonicalOrOriginal(path);
    if (std::find(deduped.begin(), deduped.end(), canon) == deduped.end()) {
      deduped.push_back(canon);
    }
  }
  return deduped;
}

std::vector<fs::path> Assets::parseEnvPaths() const {
  std::vector<fs::path> out;
  const char* raw = std::getenv(envVarName_.c_str());
  if ((raw == nullptr) || (*raw == 0)) {
    spdlog::trace("Assets env var '{}' not set", envVarName_);
    return out;
  }
  std::string value(raw);
  spdlog::trace("Assets env '{}' = '{}'", envVarName_, value);

  size_t start = 0;
  while (start <= value.size()) {
    const auto pos = value.find(kPathListDelimiter, start);
    const auto len = (pos == std::string::npos) ? value.size() - start : pos - start;
    auto token = value.substr(start, len);
    if (!token.empty()) {
      out.push_back(weaklyCanonicalOrOriginal(fs::path(token)));
    }
    if (pos == std::string::npos) {
      break;
    }
    start = pos + 1;
  }
  return out;
}

fs::path Assets::weaklyCanonicalOrOriginal(const fs::path& path) {
  std::error_code errorCode;
  auto canonical = fs::weakly_canonical(path, errorCode);
  if (!errorCode && !canonical.empty()) {
    return canonical;
  }
  return path;
}

} // namespace blkhurst
