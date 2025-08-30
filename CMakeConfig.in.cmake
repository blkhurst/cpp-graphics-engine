@PACKAGE_INIT@

include(CMakeFindDependencyMacro)

find_dependency(OpenGL REQUIRED)
find_dependency(glm 1.0.1 REQUIRED)
find_dependency(glfw3 3.4 REQUIRED)
find_dependency(spdlog 1.15.3 REQUIRED)

include("${CMAKE_CURRENT_LIST_DIR}/BlkhurstEngineTargets.cmake")
