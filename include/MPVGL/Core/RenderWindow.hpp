#pragma once

#include <string>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Shader/ShaderWatcher.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"

namespace mpvgl {

class RenderWindow {
   public:
    explicit RenderWindow(int width, int height, std::string const &title,
                          GLFWmonitor *monitor = nullptr,
                          GLFWwindow *share = nullptr) noexcept(false);

    RenderWindow(RenderWindow const &other) noexcept = delete;
    RenderWindow(RenderWindow &&other) noexcept = delete;

    RenderWindow &operator=(RenderWindow const &other) noexcept = delete;
    RenderWindow &operator=(RenderWindow &&other) noexcept = delete;

    ~RenderWindow() noexcept;

   public:
    int draw() noexcept;

   private:
    vlk::Vulkan vulkan{};
    GLFWwindow *window;
    ShaderWatcher shader_watcher;
};

}  // namespace mpvgl
