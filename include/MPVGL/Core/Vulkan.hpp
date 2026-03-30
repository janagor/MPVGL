#pragma once

#include <tl/expected.hpp>

#include "MPVGL/Core/Scene.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::vlk {

tl::expected<void, Error<EngineError>> bootstrap(
    Vulkan &vulkan, int width, int height, std::string const &title,
    GLFWmonitor *monitor, GLFWwindow *share, bool resize);
tl::expected<void, Error<EngineError>> setupRenderingPipeline(Vulkan &vulkan);
tl::expected<void, Error<EngineError>> setupRenderTargets(Vulkan &vulkan);
tl::expected<void, Error<EngineError>> loadAndPrepareAssets(Vulkan &vulkan,
                                                            Scene const &scene);
tl::expected<void, Error<EngineError>> setupDescriptorsAndSync(Vulkan &vulkan);

tl::expected<void, Error<EngineError>> createRenderPass(Vulkan &vulkan);
tl::expected<void, Error<EngineError>> createDescriptorSetLayout(
    Vulkan &vulkan);
tl::expected<void, Error<EngineError>> createGraphicsPipeline(Vulkan &vulkan);
tl::expected<void, Error<EngineError>> createCommandPool(Vulkan &vulkan);
tl::expected<void, Error<EngineError>> createDepthResources(Vulkan &vulkan);
tl::expected<void, Error<EngineError>> createFramebuffers(Vulkan &vulkan);
tl::expected<void, Error<EngineError>> loadScene(Vulkan &vulkan,
                                                 Scene const &scene);
tl::expected<void, Error<EngineError>> createUniformBuffers(Vulkan &vulkan);
tl::expected<void, Error<EngineError>> createDescriptorPool(Vulkan &vulkan);
tl::expected<void, Error<EngineError>> createCommandBuffers(Vulkan &vulkan);
tl::expected<void, Error<EngineError>> createSyncObjects(Vulkan &vulkan);
tl::expected<void, Error<EngineError>> drawFrame(Vulkan &vulkan);
tl::expected<void, Error<EngineError>> reloadShadersAndPipeline(Vulkan &vulkan);
void cleanup(Vulkan &vulkan);

// TODO: remove from public api

tl::expected<void, Error<EngineError>> deviceInitialization(
    Vulkan &vulkan, int width, int height, std::string const &title,
    GLFWmonitor *monitor, GLFWwindow *share, bool resize);
tl::expected<void, Error<EngineError>> createSwapchain(Vulkan &vulkan);
tl::expected<void, Error<EngineError>> getQueues(Vulkan &vulkan);

}  // namespace mpvgl::vlk
