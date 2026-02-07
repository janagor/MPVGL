cmake_minimum_required(VERSION 4.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_COMPILER clang++)
project(MPVGL)

find_package(Vulkan REQUIRED)

execute_process(
    COMMAND  env PYTHONPATH=. python3 update_glslang_sources.py
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/glslang"
    RESULT_VARIABLE UPDATE_GLSLANG_SOURCES_RESULT
    OUTPUT_VARIABLE UPDATE_GLSLANG_SOURCES_OUTPUT
    ERROR_VARIABLE UPDATE_GLSLANG_SOURCES_ERROR
)
if(NOT UPDATE_GLSLANG_SOURCES_RESULT EQUAL 0)
    message( FATAL_ERROR
        "update_glslang_sources.py failed:\n${UPDATE_GLSLANG_SOURCES_ERROR}"
    )
else()
    message( STATUS
        "update_glslang_sources.py succeeded:\n${UPDATE_GLSLANG_SOURCES_OUTPUT}"
    )
endif()
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/glm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/glslang)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/expected)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/vk-bootstrap)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/tinyobjloader)
if(NOT TARGET glfw::glfw)
    add_library(glfw::glfw ALIAS glfw)
endif()
if(NOT TARGET tinyobjloader::tinyobjloader)
    add_library(tinyobjloader::tinyobjloader ALIAS tinyobjloader)
endif()

add_compile_definitions(GLFW_INCLUDE_NONE)

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
    COMMENT "Formatowanie kodu przy użyciu clang-format..."
)
