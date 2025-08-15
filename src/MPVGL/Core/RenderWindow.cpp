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
    : init(), render_data(),
      shader_watcher(
          std::filesystem::path(SOURCE_DIRECTORY) / SHADERS_DIRECTORY,
          std::filesystem::path(SOURCE_DIRECTORY) / SHADERS_DIRECTORY) {

  if (!device_initialization(init).has_value())
    throw std::runtime_error("");
  if (!create_swapchain(init).has_value())
    throw std::runtime_error("");
  if (!get_queues(init, render_data).has_value())
    throw std::runtime_error("");
  if (0 != create_render_pass(init, render_data))
    throw std::runtime_error("");
  if (0 != create_graphics_pipeline(init, render_data))
    throw std::runtime_error("");
  if (0 != create_framebuffers(init, render_data))
    throw std::runtime_error("");
  if (0 != create_command_pool(init, render_data))
    throw std::runtime_error("");
  if (0 != create_vertex_buffer(init, render_data))
    throw std::runtime_error("");
  if (0 != create_index_buffer(init, render_data))
    throw std::runtime_error("");
  if (0 != create_command_buffers(init, render_data))
    throw std::runtime_error("");
  if (0 != create_sync_objects(init, render_data))
    throw std::runtime_error("");
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
      std::cout << time << std::endl;
      reloadShadersAndPipeline(init, render_data);
      time = 0;
    }
  }
  init.disp.deviceWaitIdle();
  return 0;
}

} // namespace mpvgl
