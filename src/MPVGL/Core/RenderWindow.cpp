#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/RenderWindow.hpp"
#include "MPVGL/Core/Shader/ShaderWatcher.hpp"
#include "MPVGL/Core/Vulkan.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"

#include "config.hpp"

namespace mpvgl {

RenderWindow::RenderWindow(int width, int height, std::string const &title,
                           GLFWmonitor *monitor,
                           GLFWwindow *share) noexcept(false)
    : vulkan(),
      shader_watcher(
          std::filesystem::path(SOURCE_DIRECTORY) / SHADERS_DIRECTORY,
          std::filesystem::path(SOURCE_DIRECTORY) / SHADERS_DIRECTORY) {
    shader_watcher.compileAll();

    auto result =
        vlk::bootstrap(vulkan)
            .and_then([&] { return vlk::createRenderPass(vulkan); })
            .and_then([&] { return vlk::createDescriptorSetLayout(vulkan); })
            .and_then([&] { return vlk::createGraphicsPipeline(vulkan); })
            .and_then([&] { return vlk::createCommandPool(vulkan); })
            .and_then([&] { return vlk::createDepthResources(vulkan); })
            .and_then([&] { return vlk::createFramebuffers(vulkan); })
            .and_then([&] { return vlk::createTextureImage(vulkan); })
            .and_then([&] { return vlk::createTextureImageView(vulkan); })
            .and_then([&] { return vlk::createTextureSampler(vulkan); })
            .and_then([&] { return vlk::loadModel(vulkan); })
            .and_then([&] { return vlk::createVertexBuffer(vulkan); })
            .and_then([&] { return vlk::createIndexBuffer(vulkan); })
            .and_then([&] { return vlk::createUniformBuffers(vulkan); })
            .and_then([&] { return vlk::createDescriptorPool(vulkan); })
            .and_then([&] { return vlk::createDescriptorSets(vulkan); })
            .and_then([&] { return vlk::createCommandBuffers(vulkan); })
            .and_then([&] { return vlk::createSyncObjects(vulkan); });
    if (!result.has_value()) {
        std::cerr << "[FATAL ERROR] Vulkan initialization failed: "
                  << result.error().message << "\n";
        throw std::runtime_error("Engine crash: " + result.error().message);
    }
}

RenderWindow::~RenderWindow() noexcept { cleanup(vulkan); }

int RenderWindow::draw() noexcept {
    std::jthread watcherThread(
        [&](std::stop_token st) { shader_watcher.run(st); });

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    double lastX = 400.0, lastY = 300.0;
    bool firstMouse = true;

    glfwSetInputMode(vulkan.deviceContext.window, GLFW_CURSOR,
                     GLFW_CURSOR_DISABLED);

    int time = 0;
    while (!glfwWindowShouldClose(vulkan.deviceContext.window)) {
        glfwPollEvents();

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (glfwGetKey(vulkan.deviceContext.window, GLFW_KEY_ESCAPE) ==
            GLFW_PRESS)
            glfwSetWindowShouldClose(vulkan.deviceContext.window, true);

        if (glfwGetKey(vulkan.deviceContext.window, GLFW_KEY_W) == GLFW_PRESS)
            vulkan.sceneContext.camera.processKeyboard(CameraMovement::Forward,
                                                       deltaTime);
        if (glfwGetKey(vulkan.deviceContext.window, GLFW_KEY_S) == GLFW_PRESS)
            vulkan.sceneContext.camera.processKeyboard(CameraMovement::Backward,
                                                       deltaTime);
        if (glfwGetKey(vulkan.deviceContext.window, GLFW_KEY_A) == GLFW_PRESS)
            vulkan.sceneContext.camera.processKeyboard(CameraMovement::Left,
                                                       deltaTime);
        if (glfwGetKey(vulkan.deviceContext.window, GLFW_KEY_D) == GLFW_PRESS)
            vulkan.sceneContext.camera.processKeyboard(CameraMovement::Right,
                                                       deltaTime);
        if (glfwGetKey(vulkan.deviceContext.window, GLFW_KEY_SPACE) ==
            GLFW_PRESS)
            vulkan.sceneContext.camera.processKeyboard(CameraMovement::Up,
                                                       deltaTime);
        if (glfwGetKey(vulkan.deviceContext.window, GLFW_KEY_LEFT_SHIFT) ==
            GLFW_PRESS)
            vulkan.sceneContext.camera.processKeyboard(CameraMovement::Down,
                                                       deltaTime);
        if (glfwGetKey(vulkan.deviceContext.window, GLFW_KEY_Q) == GLFW_PRESS)
            vulkan.sceneContext.camera.processKeyboard(CameraMovement::RollLeft,
                                                       deltaTime);
        if (glfwGetKey(vulkan.deviceContext.window, GLFW_KEY_E) == GLFW_PRESS)
            vulkan.sceneContext.camera.processKeyboard(
                CameraMovement::RollRight, deltaTime);

        double xpos, ypos;
        glfwGetCursorPos(vulkan.deviceContext.window, &xpos, &ypos);

        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        lastX = xpos;
        lastY = ypos;

        vulkan.sceneContext.camera.processMouseMovement(xoffset, yoffset);

        if (auto result = vlk::drawFrame(vulkan); !result.has_value()) {
            std::cout << "failed to draw a Frame:" << result.error().message
                      << std::endl;
            return -1;
        }

        if (time++ >= 10'000) {
            if (auto result = vlk::reloadShadersAndPipeline(vulkan);
                !result.has_value()) {
                std::cout << result.error().message << "\n";
                vulkan.deviceContext.logDevDisp.deviceWaitIdle();
                return 0;
            }
            time = 0;
        }
    }

    vulkan.deviceContext.logDevDisp.deviceWaitIdle();
    return 0;
}

}  // namespace mpvgl
