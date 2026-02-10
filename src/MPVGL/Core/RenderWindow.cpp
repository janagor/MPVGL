#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/RenderWindow.hpp"
#include "MPVGL/Core/Shader/ShaderWatcher.hpp"
#include "MPVGL/Core/Vulkan.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"

#include "config.hpp"

namespace mpvgl {

RenderWindow::RenderWindow(int width, int height, std::string const &title,
                           GLFWmonitor *monitor,
                           GLFWwindow *share) noexcept(false)
    : vulkan(),
      shader_watcher(
          std::filesystem::path(SOURCE_DIRECTORY) / SHADERS_DIRECTORY,
          std::filesystem::path(SOURCE_DIRECTORY) / SHADERS_DIRECTORY) {
    shader_watcher.compileAll();

    if (auto result = vlk::bootstrap(vulkan); !result.has_value()) {
        std::cout << result.error().message() << "\n";
        throw std::runtime_error("Failed .");
    }
    if (0 != vlk::create_render_pass(vulkan))
        throw std::runtime_error("Failed at `create_render_pass`.");
    if (0 != vlk::create_descriptor_set_layout(vulkan))
        throw std::runtime_error("Failed at `create_descriptor_set_layout`.");
    if (0 != vlk::create_graphics_pipeline(vulkan))
        throw std::runtime_error("Failed at `create_graphics_pipeline`.");
    if (0 != vlk::create_command_pool(vulkan))
        throw std::runtime_error("Failed at `create_command_pool`.");
    if (0 != vlk::create_depth_resources(vulkan))
        throw std::runtime_error("Failed at `create_depth_resources`.");
    if (0 != vlk::create_framebuffers(vulkan))
        throw std::runtime_error("Failed at `create_framebuffers`.");
    if (0 != vlk::create_texture_image(vulkan))
        throw std::runtime_error("Failed at `create_texture_image`.");
    if (0 != vlk::create_texture_image_view(vulkan))
        throw std::runtime_error("Failed at `create_texture_image_view`.");
    if (0 != vlk::create_texture_sampler(vulkan))
        throw std::runtime_error("Failed at `create_texture_sampler`.");
    if (0 != vlk::load_model(vulkan))
        throw std::runtime_error("Failed at `load_model`.");
    if (0 != vlk::create_vertex_buffer(vulkan))
        throw std::runtime_error("Failed at `create_vertex_buffer`.");
    if (0 != vlk::create_index_buffer(vulkan))
        throw std::runtime_error("Failed at `create_index_buffer`.");
    if (0 != vlk::create_uniform_buffers(vulkan))
        throw std::runtime_error("Failed at `create_uniform_buffers`.");
    if (0 != vlk::create_descriptor_pool(vulkan))
        throw std::runtime_error("Failed at `create_descriptor_pool`.");
    if (0 != vlk::create_descriptor_sets(vulkan))
        throw std::runtime_error("Failed at `create_descriptor_sets`.");
    if (0 != vlk::create_command_buffers(vulkan))
        throw std::runtime_error("Failed at `create_command_buffers`.");
    if (0 != vlk::create_sync_objects(vulkan))
        throw std::runtime_error("Failed at `create_sync_objects`.");
}

RenderWindow::~RenderWindow() noexcept { cleanup(vulkan); }

int RenderWindow::draw() noexcept {
    std::jthread watcherThread(
        [&](std::stop_token st) { shader_watcher.run(st); });

    int time = 0;
    while (!glfwWindowShouldClose(vulkan.window)) {
        glfwPollEvents();
        int res = vlk::draw_frame(vulkan);
        if (res != 0) {
            std::cout << "failed to draw frame \n";
            return -1;
        }
        if (time++ >= 10'000) {
            vlk::reloadShadersAndPipeline(vulkan);
            time = 0;
        }
    }
    vulkan.dispatchTable.deviceWaitIdle();
    return 0;
}

}  // namespace mpvgl
