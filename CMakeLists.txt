cmake_minimum_required(VERSION 4.0)
project(MPVGL)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(Vulkan REQUIRED)
add_subdirectory(lib/vk-bootstrap)
add_subdirectory(lib/glfw)
add_subdirectory(lib/glm)

configure_file (
    "${PROJECT_SOURCE_DIR}/config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.hpp"
)

file(GLOB_RECURSE MPVGL_SRC ${PROJECT_SOURCE_DIR}/src/MPVGL/*.cpp)
add_library(${PROJECT_NAME} STATIC ${MPVGL_SRC})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)

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
