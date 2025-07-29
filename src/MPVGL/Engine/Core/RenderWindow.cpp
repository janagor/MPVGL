#include <iostream>
#include <stdexcept>
#include <string>

#include <GLFW/glfw3.h>

#include "MPVGL/Engine/Core/RenderWindow.hpp"
#include "MPVGL/Engine/Core/Vulkan.hpp"

namespace mpvgl {

RenderWindow::RenderWindow(int width, int height, std::string const &title,
                           GLFWmonitor *monitor,
                           GLFWwindow *share) noexcept(false)
    : init(), render_data() {

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

RenderWindow::RenderWindow(RenderWindow &&other) noexcept
    : window(other.window), init(other.init), render_data(other.render_data) {
  other.window = nullptr;
  other.init = Init();
  other.render_data = RenderData();
}

RenderWindow &RenderWindow::operator=(RenderWindow &&other) noexcept {
  this->window = other.window;
  other.window = nullptr;
  this->init = other.init;
  other.init = Init();
  this->render_data = other.render_data;
  other.render_data = RenderData();
  return (*this);
}

RenderWindow::~RenderWindow() noexcept { cleanup(init, render_data); }

int RenderWindow::draw() noexcept {
  while (!glfwWindowShouldClose(init.window)) {
    glfwPollEvents();
    int res = draw_frame(init, render_data);
    if (res != 0) {
      std::cout << "failed to draw frame \n";
      return -1;
    }
  }
  init.disp.deviceWaitIdle();
  return 0;
}

} // namespace mpvgl
