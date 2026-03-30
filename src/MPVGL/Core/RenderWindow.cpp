#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <thread>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/InputManager.hpp"
#include "MPVGL/Core/RenderWindow.hpp"
#include "MPVGL/Core/Scene.hpp"
#include "MPVGL/Core/Shader/ShaderWatcher.hpp"
#include "MPVGL/Core/Vulkan.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"

#include "config.hpp"

namespace mpvgl {

RenderWindow::RenderWindow(int width, int height, std::string const &title,
                           Scene const &scene, GLFWmonitor *monitor,
                           GLFWwindow *share) noexcept(false)
    : shader_watcher(
          std::filesystem::path(SOURCE_DIRECTORY) / SHADERS_DIRECTORY,
          std::filesystem::path(SOURCE_DIRECTORY) / SHADERS_DIRECTORY) {
    shader_watcher.compileAll();

    auto result =
        vlk::bootstrap(vulkan)
            .and_then([&] { return vlk::setupRenderingPipeline(vulkan); })
            .and_then([&] { return vlk::setupRenderTargets(vulkan); })
            .and_then([&] { return vlk::setupDescriptorsAndSync(vulkan); })
            .and_then([&] { return vlk::loadAndPrepareAssets(vulkan, scene); });
    if (!result.has_value()) {
        std::cerr << "[FATAL ERROR] Vulkan initialization failed: "
                  << result.error().message << "\n";
        throw std::runtime_error("Engine crash: " + result.error().message);
    }
}

RenderWindow::~RenderWindow() noexcept { cleanup(vulkan); }

int RenderWindow::draw() noexcept {
    std::jthread watcherThread(
        [&](std::stop_token const &st) { shader_watcher.run(st); });

    f32 deltaTime = 0.0f;
    f32 lastFrame = 0.0f;

    f64 lastX = 400.0, lastY = 300.0;
    bool firstMouse = true;

    glfwSetInputMode(vulkan.deviceContext.window, GLFW_CURSOR,
                     GLFW_CURSOR_DISABLED);

    int time = 0;
    while (!glfwWindowShouldClose(vulkan.deviceContext.window)) {
        glfwPollEvents();

        f32 currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        int i = 0;
        for (auto &object : vulkan.sceneContext.renderables) {
            f32 speed = (i % 2 == 0) ? 45.0f : -90.0f;
            object.transformMatrix = glm::rotate(
                object.transformMatrix, deltaTime * glm::radians(speed),
                glm::vec3(0.0f, 0.0f, 1.0f));
            i++;
        }

        InputManager::processKeyboard(vulkan.deviceContext.window,
                                      vulkan.sceneContext.camera, deltaTime);
        InputManager::processMouse(vulkan.deviceContext.window,
                                   vulkan.sceneContext.camera, lastX, lastY,
                                   firstMouse);

        if (auto result = vlk::drawFrame(vulkan); !result.has_value()) {
            std::cout << "failed to draw a Frame:" << result.error().message
                      << '\n';
            return -1;
        }

        if (shader_watcher.consumeChange()) {
            vulkan.deviceContext.logDevDisp.deviceWaitIdle();
            if (auto result = vlk::reloadShadersAndPipeline(vulkan);
                !result.has_value()) {
                std::cerr << "[Hot-Reload] Pipeline creation failed: "
                          << result.error().message << "\n";
            } else {
                std::cout << "[Hot-Reload] Pipeline successfully updated in "
                             "real-time!\n";
            }
        }
    }

    vulkan.deviceContext.logDevDisp.deviceWaitIdle();
    return 0;
}

}  // namespace mpvgl
