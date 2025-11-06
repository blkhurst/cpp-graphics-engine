if (WIN32)
  set(ICON blkhurst.rc)
endif()

add_executable(example example.cpp ${ICON}) # WIN32
target_link_libraries(example PRIVATE BlkhurstEngine)
