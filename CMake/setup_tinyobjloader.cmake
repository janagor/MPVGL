add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/tinyobjloader)
if(NOT TARGET tinyobjloader::tinyobjloader)
    add_library(tinyobjloader::tinyobjloader ALIAS tinyobjloader)
endif()
