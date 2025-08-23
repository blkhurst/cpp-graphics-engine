@PACKAGE_INIT@

include(CMakeFindDependencyMacro)

find_dependency(OpenGL REQUIRED)
find_dependency(glfw3 3.4 REQUIRED)

include("${CMAKE_CURRENT_LIST_DIR}/BlkhurstEngineTargets.cmake")
