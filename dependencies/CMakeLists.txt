# https://github.com/Dav1dde/glad
set(GLAD_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/glad/gl.c
  PARENT_SCOPE
)

set(GLAD_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/glad
  PARENT_SCOPE
)

# https://github.com/ocornut/imgui/wiki/Getting-Started#compilinglinking
set(IMGUI_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/misc/cpp/imgui_stdlib.cpp
  PARENT_SCOPE
)

set(IMGUI_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
  PARENT_SCOPE
)

# 
set(STB_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/stb
  PARENT_SCOPE
)
