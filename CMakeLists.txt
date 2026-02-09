cmake_minimum_required(VERSION 4.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_COMPILER clang++)
project(MPVGL)

find_package(Vulkan REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

include(setup_glfw)
include(setup_glslang)
include(setup_expected)
include(setup_tinyobjloader)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/glm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/vk-bootstrap)

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.hpp"
)
configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/.clangd.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/.clangd"
)

file(GLOB_RECURSE MPVGL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/MPVGL/*.cpp)
add_library(${PROJECT_NAME} STATIC ${MPVGL_SRC})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/stb
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    glm::glm
    glfw::glfw
    tl::expected
    Vulkan::Vulkan
    glslang::SPIRV
    glslang::glslang
    vk-bootstrap::vk-bootstrap
    tinyobjloader::tinyobjloader
    glslang::glslang-default-resource-limits
)

add_subdirectory(examples)

file(GLOB_RECURSE ALL_SOURCE_FILES "src/*.cpp" "include/*.hpp")
add_custom_target(format
    COMMAND clang-format -i ${ALL_SOURCE_FILES}
    COMMENT "Formatowanie code..."
)
