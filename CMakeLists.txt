cmake_minimum_required(VERSION 4.0)
project(MPVGL)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_subdirectory(lib/vk-bootstrap)
add_subdirectory(lib/glfw)

configure_file (
    "${PROJECT_SOURCE_DIR}/config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.hpp"
)

add_executable(main main.cpp config.hpp)
target_link_libraries(main PUBLIC
    vk-bootstrap::vk-bootstrap
    glfw
)
target_include_directories(main PUBLIC
    ${CMAKE_BINARY_DIR}
)
