#pragma once

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Camera.hpp"

namespace mpvgl {

class InputManager {
   public:
    static void processKeyboard(GLFWwindow* window, Camera& camera,
                                float deltaTime);

    static void processMouse(GLFWwindow* window, Camera& camera, double& lastX,
                             double& lastY, bool& firstMouse);
};

}  // namespace mpvgl
