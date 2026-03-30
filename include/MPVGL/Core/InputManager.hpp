#pragma once

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Camera.hpp"

namespace mpvgl {

class InputManager {
   public:
    void processKeyboard(GLFWwindow* window, Camera& camera, f64 deltaTime);
    void processMouse(GLFWwindow* window, Camera& camera);

   private:
    f64 m_lastX{0.0};
    f64 m_lastY{0.0};
    bool m_firstMouse{true};
};

}  // namespace mpvgl
