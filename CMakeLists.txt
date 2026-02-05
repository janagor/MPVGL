cmake_minimum_required(VERSION 4.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_COMPILER clang++)
project(MPVGL)

find_package(Vulkan REQUIRED)

add_subdirectory(${PROJECT_SOURCE_DIR}/lib/vk-bootstrap)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/expected)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/glfw)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/glm)

add_compile_definitions(GLFW_INCLUDE_NONE)

##
set(GLSLANG_DIR "${PROJECT_SOURCE_DIR}/lib/glslang")
execute_process(
  COMMAND  env PYTHONPATH=. python3 update_glslang_sources.py
  WORKING_DIRECTORY ${GLSLANG_DIR}
  RESULT_VARIABLE UPDATE_GLSLANG_SOURCES_RESULT
  OUTPUT_VARIABLE UPDATE_GLSLANG_SOURCES_OUTPUT
  ERROR_VARIABLE UPDATE_GLSLANG_SOURCES_ERROR
)
if(NOT UPDATE_GLSLANG_SOURCES_RESULT EQUAL 0)
  message(FATAL_ERROR "update_glslang_sources.py failed:\n${UPDATE_GLSLANG_SOURCES_ERROR}")
else()
  message(STATUS "update_glslang_sources.py succeeded:\n${UPDATE_GLSLANG_SOURCES_OUTPUT}")
endif()
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/glslang)

configure_file (
    "${PROJECT_SOURCE_DIR}/config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.hpp"
)

file(GLOB_RECURSE MPVGL_SRC ${PROJECT_SOURCE_DIR}/src/MPVGL/*.cpp)
add_library(${PROJECT_NAME} STATIC ${MPVGL_SRC})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/lib/glm
    ${PROJECT_SOURCE_DIR}/lib/glslang
    ${PROJECT_SOURCE_DIR}/lib/glfw/include
    ${PROJECT_SOURCE_DIR}/lib/vk-bootstrap/src
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    glfw
    glm::glm
    tl::expected
    Vulkan::Vulkan
    glslang::SPIRV
    glslang::glslang
    vk-bootstrap::vk-bootstrap
    glslang::glslang-default-resource-limits
)

set(EXECUTABLE_NAME "main")

add_executable(${EXECUTABLE_NAME}
    main.cpp
    config.hpp
)

target_link_libraries(${EXECUTABLE_NAME} PUBLIC
    ${PROJECT_NAME}
)

target_include_directories(${EXECUTABLE_NAME} PUBLIC
	${PROJECT_SOURCE_DIR}/include
)
