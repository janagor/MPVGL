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
                                   float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    for (auto const& [key, movement] : keyMappings) {
        if (glfwGetKey(window, key) == GLFW_PRESS) {
            camera.processKeyboard(movement, deltaTime);
        }
    }
}

void InputManager::processMouse(GLFWwindow* window, Camera& camera,
                                double& lastX, double& lastY,
                                bool& firstMouse) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos - lastX);
    float yoffset = static_cast<float>(ypos - lastY);
    lastX = xpos;
    lastY = ypos;

    camera.processMouseMovement(xoffset, yoffset);
}

}  // namespace mpvgl
