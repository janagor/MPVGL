#include <array>
#include <utility>

#include "MPVGL/Core/InputManager.hpp"

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
                                   f32 deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    for (auto const& [key, movement] : keyMappings) {
        if (glfwGetKey(window, key) == GLFW_PRESS) {
            camera.processKeyboard(movement, deltaTime);
        }
    }
}

void InputManager::processMouse(GLFWwindow* window, Camera& camera, f64& lastX,
                                f64& lastY, bool& firstMouse) {
    f64 xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    f32 xoffset = static_cast<f32>(xpos - lastX);
    f32 yoffset = static_cast<f32>(ypos - lastY);
    lastX = xpos;
    lastY = ypos;

    camera.processMouseMovement(xoffset, yoffset);
}

}  // namespace mpvgl
