set(EXAMPLE_NAME "mpvgl_example")

add_executable(${EXAMPLE_NAME} main.cpp)

target_link_libraries(${EXAMPLE_NAME} PRIVATE ${PROJECT_NAME})
