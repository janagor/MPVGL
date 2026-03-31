#include <filesystem>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <thread>

#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <tl/expected.hpp>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/InputManager.hpp"
#include "MPVGL/Core/RenderWindow.hpp"
#include "MPVGL/Core/Scene.hpp"
#include "MPVGL/Core/Shader/ShaderWatcher.hpp"
#include "MPVGL/Core/Vulkan.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"
#include "MPVGL/Utility/Types.hpp"

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
        vlk::bootstrap(vulkan, width, height, title, monitor, share, true)
            .and_then([&] -> tl::expected<void, Error<EngineError>> {
                return vlk::setupRenderingPipeline(vulkan);
            })
            .and_then([&] -> tl::expected<void, Error<EngineError>> {
                return vlk::setupRenderTargets(vulkan);
            })
            .and_then([&] -> tl::expected<void, Error<EngineError>> {
                return vlk::setupDescriptorsAndSync(vulkan);
            })
            .and_then([&] -> tl::expected<void, Error<EngineError>> {
                return vlk::loadAndPrepareAssets(vulkan, scene);
            });
    if (!result.has_value()) {
        std::cerr << "[FATAL ERROR] Vulkan initialization failed: "
                  << result.error().message() << "\n";
        throw std::runtime_error("Engine crash: " + result.error().message());
    }
}

RenderWindow::~RenderWindow() noexcept { cleanup(vulkan); }

auto RenderWindow::draw() noexcept -> int {
    std::jthread const watcherThread(
        [&](std::stop_token const &stopToken) -> void {
            shader_watcher.run(stopToken);
        });

    f64 deltaTime = 0.0;
    f64 lastFrame = 0.0;

    glfwSetInputMode(vulkan.deviceContext.window, GLFW_CURSOR,
                     GLFW_CURSOR_DISABLED);

    while (glfwWindowShouldClose(vulkan.deviceContext.window) == GLFW_FALSE) {
        glfwPollEvents();

        auto const currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        for (auto &&[index, object] :
             vulkan.sceneContext.renderables | std::views::enumerate) {
            f32 const speed = (index % 2 == 0) ? 45.0F : -90.0F;
            object.transformMatrix =
                glm::rotate(object.transformMatrix,
                            static_cast<f32>(deltaTime) * glm::radians(speed),
                            glm::vec3(0.0F, 0.0F, 1.0F));
        }

        InputManager::processKeyboard(vulkan.deviceContext.window,
                                      vulkan.sceneContext.camera, deltaTime);
        m_inputManager.processMouse(vulkan.deviceContext.window,
                                    vulkan.sceneContext.camera);

        if (auto result = vlk::drawFrame(vulkan); !result.has_value()) {
            std::cout << "failed to draw a Frame:" << result.error().message()
                      << '\n';
            return -1;
        }

        if (shader_watcher.consumeChange()) {
            vulkan.deviceContext.logDevDisp.deviceWaitIdle();
            if (auto result = vlk::reloadShadersAndPipeline(vulkan);
                !result.has_value()) {
                std::cerr << "[Hot-Reload] Pipeline creation failed: "
                          << result.error().message() << "\n";
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
