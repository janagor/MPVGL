#pragma once

#include <string>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/InputManager.hpp"
#include "MPVGL/Core/Scene.hpp"
#include "MPVGL/Core/Shader/ShaderWatcher.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"

namespace mpvgl {

class RenderWindow {
   public:
    explicit RenderWindow(int width, int height, std::string const &title,
                          Scene const &scene, GLFWmonitor *monitor = nullptr,
                          GLFWwindow *share = nullptr) noexcept(false);

    RenderWindow(RenderWindow const &other) noexcept = delete;
    RenderWindow(RenderWindow &&other) noexcept = delete;

    auto operator=(RenderWindow const &other) noexcept
        -> RenderWindow & = delete;
    auto operator=(RenderWindow &&other) noexcept -> RenderWindow & = delete;

    ~RenderWindow() noexcept;

    auto draw() noexcept -> int;

   private:
    vlk::Vulkan vulkan{};
    GLFWwindow *window = nullptr;
    ShaderWatcher shader_watcher;
    InputManager m_inputManager{};
};

}  // namespace mpvgl
