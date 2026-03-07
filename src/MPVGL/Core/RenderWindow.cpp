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

    if (auto result = vlk::bootstrap(vulkan); !result.has_value()) {
        std::cout << result.error().message << "\n";
        throw std::runtime_error("Failed .");
    }
    if (auto result = vlk::createRenderPass(vulkan); !result.has_value()) {
        std::cout << result.error().message << "\n";
        throw std::runtime_error("Failed .");
    }
    if (auto result = vlk::createDescriptorSetLayout(vulkan);
        !result.has_value()) {
        std::cout << result.error().message << "\n";
        throw std::runtime_error("Failed .");
    }
    if (auto result = vlk::createGraphicsPipeline(vulkan);
        !result.has_value()) {
        std::cout << result.error().message << "\n";
        throw std::runtime_error("Failed .");
    }
    if (0 != vlk::create_command_pool(vulkan))
        throw std::runtime_error("Failed at `create_command_pool`.");
    if (0 != vlk::create_depth_resources(vulkan))
        throw std::runtime_error("Failed at `create_depth_resources`.");
    if (0 != vlk::create_framebuffers(vulkan))
        throw std::runtime_error("Failed at `create_framebuffers`.");
    if (0 != vlk::create_texture_image(vulkan))
        throw std::runtime_error("Failed at `create_texture_image`.");
    if (0 != vlk::create_texture_image_view(vulkan))
        throw std::runtime_error("Failed at `create_texture_image_view`.");
    if (0 != vlk::create_texture_sampler(vulkan))
        throw std::runtime_error("Failed at `create_texture_sampler`.");
    if (0 != vlk::load_model(vulkan))
        throw std::runtime_error("Failed at `load_model`.");
    if (0 != vlk::create_vertex_buffer(vulkan))
        throw std::runtime_error("Failed at `create_vertex_buffer`.");
    if (0 != vlk::create_index_buffer(vulkan))
        throw std::runtime_error("Failed at `create_index_buffer`.");
    if (0 != vlk::create_uniform_buffers(vulkan))
        throw std::runtime_error("Failed at `create_uniform_buffers`.");
    if (0 != vlk::create_descriptor_pool(vulkan))
        throw std::runtime_error("Failed at `create_descriptor_pool`.");
    if (0 != vlk::create_descriptor_sets(vulkan))
        throw std::runtime_error("Failed at `create_descriptor_sets`.");
    if (0 != vlk::create_command_buffers(vulkan))
        throw std::runtime_error("Failed at `create_command_buffers`.");
    if (0 != vlk::create_sync_objects(vulkan))
        throw std::runtime_error("Failed at `create_sync_objects`.");
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

        int res = vlk::draw_frame(vulkan);
        if (res != 0) {
            std::cout << "failed to draw frame \n";
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
