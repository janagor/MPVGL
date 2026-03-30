#pragma once

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Camera.hpp"

namespace mpvgl {

class InputManager {
   public:
    static void processKeyboard(GLFWwindow* window, Camera& camera,
                                f32 deltaTime);

    static void processMouse(GLFWwindow* window, Camera& camera, f64& lastX,
                             f64& lastY, bool& firstMouse);
};

}  // namespace mpvgl
