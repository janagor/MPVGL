cmake_minimum_required(VERSION 4.0)
project(MPVGL)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(Vulkan REQUIRED)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/vk-bootstrap)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/glfw)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/glm)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/expected)

configure_file (
    "${PROJECT_SOURCE_DIR}/config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.hpp"
)

file(GLOB_RECURSE MPVGL_SRC ${PROJECT_SOURCE_DIR}/src/MPVGL/*.cpp)
add_library(${PROJECT_NAME} STATIC ${MPVGL_SRC} )
target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    vk-bootstrap::vk-bootstrap
    glfw
    glm::glm
    Vulkan::Vulkan
    tl::expected
)
target_include_directories(${PROJECT_NAME} PUBLIC
    vk-bootstrap::vk-bootstrap
    glfw
    glm::glm
    Vulkan::Vulkan
    tl::expected
)

set(EXECUTABLE_NAME "main")

add_executable(${EXECUTABLE_NAME}
    main.cpp
    config.hpp
)
target_link_libraries(${EXECUTABLE_NAME} PUBLIC
    ${PROJECT_NAME}
    vk-bootstrap::vk-bootstrap
    glfw
    glm::glm
    Vulkan::Vulkan
)
target_include_directories(${EXECUTABLE_NAME} PUBLIC
    ${PROJECT_NAME}/include
    ${CMAKE_BINARY_DIR}
)
