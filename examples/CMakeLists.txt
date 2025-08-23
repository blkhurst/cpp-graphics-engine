if (WIN32)
  set(ICON BlkhurstEngine.rc)
endif()

add_executable(example WIN32 example.cpp ${ICON})
target_link_libraries(example PRIVATE BlkhurstEngine)
