#include "MPVGL/Core/RenderWindow.hpp"

#include <GLFW/glfw3.h>

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include "MPVGL/Core/Shader/ShaderWatcher.hpp"
#include "MPVGL/Core/Vulkan.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"
#include "config.hpp"

namespace mpvgl {

RenderWindow::RenderWindow(int width, int height, std::string const &title,
                           GLFWmonitor *monitor,
                           GLFWwindow *share) noexcept(false)
    : init(),
      render_data(),
      shader_watcher(
          std::filesystem::path(SOURCE_DIRECTORY) / SHADERS_DIRECTORY,
          std::filesystem::path(SOURCE_DIRECTORY) / SHADERS_DIRECTORY) {
  shader_watcher.compileAll();

  if (!device_initialization(init).has_value())
    throw std::runtime_error("Failed at `device_initialization`.");
  if (!create_swapchain(init).has_value())
    throw std::runtime_error("Failed at `create_swapchain`.");
  if (!get_queues(init, render_data).has_value())
    throw std::runtime_error("Failed at `get_queues`.");
  if (0 != create_render_pass(init, render_data))
    throw std::runtime_error("Failed at `create_render_pass`.");
  if (0 != create_descriptor_set_layout(init, render_data))
    throw std::runtime_error("Failed at `create_descriptor_set_layout`.");
  if (0 != create_graphics_pipeline(init, render_data))
    throw std::runtime_error("Failed at `create_graphics_pipeline`.");
  if (0 != create_framebuffers(init, render_data))
    throw std::runtime_error("Failed at `create_framebuffers`.");
  if (0 != create_command_pool(init, render_data))
    throw std::runtime_error("Failed at `create_command_pool`.");
  if (0 != create_texture_image(init, render_data))
    throw std::runtime_error("Failed at `create_texture_image`.");
  if (0 != create_texture_image_view(init, render_data))
    throw std::runtime_error("Failed at `create_texture_image_view`.");
  if (0 != create_texture_sampler(init, render_data))
    throw std::runtime_error("Failed at `create_texture_sampler`.");
  if (0 != create_vertex_buffer(init, render_data))
    throw std::runtime_error("Failed at `create_vertex_buffer`.");
  if (0 != create_index_buffer(init, render_data))
    throw std::runtime_error("Failed at `create_index_buffer`.");
  if (0 != create_uniform_buffers(init, render_data))
    throw std::runtime_error("Failed at `create_uniform_buffers`.");
  if (0 != create_descriptor_pool(init, render_data))
    throw std::runtime_error("Failed at `create_descriptor_pool`.");
  if (0 != create_descriptor_sets(init, render_data))
    throw std::runtime_error("Failed at `create_descriptor_sets`.");
  if (0 != create_command_buffers(init, render_data))
    throw std::runtime_error("Failed at `create_command_buffers`.");
  if (0 != create_sync_objects(init, render_data))
    throw std::runtime_error("Failed at `create_sync_objects`.");
}

RenderWindow::~RenderWindow() noexcept { cleanup(init, render_data); }

int RenderWindow::draw() noexcept {
  std::jthread watcherThread(
      [&](std::stop_token st) { shader_watcher.run(st); });

  int time = 0;
  while (!glfwWindowShouldClose(init.window)) {
    glfwPollEvents();
    int res = draw_frame(init, render_data);
    if (res != 0) {
      std::cout << "failed to draw frame \n";
      return -1;
    }
    if (time++ >= 10'000) {
      reloadShadersAndPipeline(init, render_data);
      time = 0;
    }
  }
  init.disp.deviceWaitIdle();
  return 0;
}

}  // namespace mpvgl
