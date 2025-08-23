cmake_minimum_required(VERSION 3.26)
project(BlkhurstEngine VERSION 0.1.0 LANGUAGES C CXX)

# Export `compile_commands.json` for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BLKHURST_USE_FETCHCONTENT "Fetch deps automatically" OFF)
option(BLKHURST_BUILD_EXAMPLES "Build examples" ON)
option(BLKHURST_INSTALL "Generate installation target" ON)

# Dependencies
find_package(OpenGL REQUIRED)

if (BLKHURST_USE_FETCHCONTENT)
  include(FetchContent)
  FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
  )
  FetchContent_MakeAvailable(glfw)
else()
  find_package(glfw3 3.4 REQUIRED)
endif()

add_subdirectory(dependencies)

# Library Source
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(BlkhurstEngine STATIC ${SOURCES})

target_sources(BlkhurstEngine PRIVATE
  ${GLAD_SOURCES}
  ${IMGUI_SOURCES}
)

# Library Public / Private Include Paths
target_include_directories(BlkhurstEngine
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    "$<BUILD_INTERFACE:${IMGUI_INCLUDE_DIRS}>"
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/BlkhurstEngine>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/BlkhurstEngine/imgui>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GLAD_INCLUDE_DIRS}
    ${STB_INCLUDE_DIRS}
)

# Link Libraries - Static library requires PUBLIC so consumers auto-link. Shared library use PRIVATE unless exposed in public headers.
target_link_libraries(BlkhurstEngine PUBLIC
  OpenGL::GL
  glfw
)

# Compile Features
target_compile_features(BlkhurstEngine PUBLIC cxx_std_17)

# Examples
if (BLKHURST_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Install
if (BLKHURST_INSTALL)
  include(GNUInstallDirs)

  install(TARGETS BlkhurstEngine 
    EXPORT BlkhurstEngineTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/BlkhurstEngine
  )

  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/imgui/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/BlkhurstEngine/imgui
  )

  install(EXPORT BlkhurstEngineTargets
    FILE BlkhurstEngineTargets.cmake
    NAMESPACE 
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BlkhurstEngine
  )

  include(CMakePackageConfigHelpers)

  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/CMakeConfig.in.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/BlkhurstEngineConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BlkhurstEngine
  )

  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/BlkhurstEngineConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/BlkhurstEngineConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/BlkhurstEngineConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BlkhurstEngine
  )
endif()
