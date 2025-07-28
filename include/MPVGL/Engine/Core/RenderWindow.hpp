#pragma once

#include <string>

#include <GLFW/glfw3.h>

#include "MPVGL/Engine/Core/Vulkan.hpp"

namespace mpvgl {

class RenderWindow {
public:
  explicit RenderWindow(int width, int height, std::string const &title,
                        GLFWmonitor *monitor = nullptr,
                        GLFWwindow *share = nullptr) noexcept(false);

  RenderWindow(RenderWindow const &other) noexcept = delete;
  RenderWindow(RenderWindow &&other) noexcept;

  RenderWindow &operator=(RenderWindow const &other) noexcept = delete;
  RenderWindow &operator=(RenderWindow &&other) noexcept;

  ~RenderWindow() noexcept;

public:
  int draw() noexcept;

private:
  Init init;
  RenderData render_data;
  GLFWwindow *window;
};

} // namespace mpvgl
