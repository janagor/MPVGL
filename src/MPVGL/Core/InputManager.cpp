#include <math.h>

#include <array>
#include <utility>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Camera.hpp"
#include "MPVGL/Core/InputManager.hpp"
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl {

namespace {

constexpr std::array<std::pair<int, CameraMovement>, 8> keyMappings = {
    {{GLFW_KEY_W, CameraMovement::Forward},
     {GLFW_KEY_S, CameraMovement::Backward},
     {GLFW_KEY_A, CameraMovement::Left},
     {GLFW_KEY_D, CameraMovement::Right},
     {GLFW_KEY_SPACE, CameraMovement::Up},
     {GLFW_KEY_LEFT_SHIFT, CameraMovement::Down},
     {GLFW_KEY_Q, CameraMovement::RollLeft},
     {GLFW_KEY_E, CameraMovement::RollRight}}};

}

void InputManager::processKeyboard(GLFWwindow* window, Camera& camera,
                                   f64 deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    for (auto const& [key, movement] : keyMappings) {
        if (glfwGetKey(window, key) == GLFW_PRESS) {
            camera.processKeyboard(movement, deltaTime);
        }
    }
}

void InputManager::processMouse(GLFWwindow* window, Camera& camera) {
    f64 xpos{};
    f64 ypos{};
    glfwGetCursorPos(window, &xpos, &ypos);

    if (m_firstMouse) {
        m_lastX = xpos;
        m_lastY = ypos;
        m_firstMouse = false;
    }

    auto const xoffset = xpos - m_lastX;
    auto const yoffset = ypos - m_lastY;

    m_lastX = xpos;
    m_lastY = ypos;

    camera.processMouseMovement(xoffset, yoffset);
}

}  // namespace mpvgl
