#pragma once

#include <string>
#include <vector>

namespace blkhurst {

struct AssetsConfig {
  std::string installRoot;
  std::vector<std::string> searchPaths;
  // TODO: useCwd, useExeDir, envVar, verbose
};

} // namespace blkhurst
