#pragma once

#include <tl/expected.hpp>

#include "MPVGL/Core/Scene.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::vlk {

auto bootstrap(Vulkan &vulkan, int width, int height, std::string const &title,
               GLFWmonitor *monitor, GLFWwindow *share, bool resize)
    -> tl::expected<void, Error<EngineError>>;
auto setupRenderingPipeline(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>>;
auto setupRenderTargets(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>>;
auto loadAndPrepareAssets(Vulkan &vulkan, Scene const &scene)
    -> tl::expected<void, Error<EngineError>>;
auto setupDescriptorsAndSync(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>>;

auto createRenderPass(Vulkan &vulkan) -> tl::expected<void, Error<EngineError>>;
auto createDescriptorSetLayout(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>>;
auto createGraphicsPipeline(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>>;
auto createCommandPool(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>>;
auto createDepthResources(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>>;
auto createFramebuffers(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>>;
auto loadScene(Vulkan &vulkan, Scene const &scene)
    -> tl::expected<void, Error<EngineError>>;
auto createUniformBuffers(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>>;
auto createDescriptorPool(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>>;
auto createCommandBuffers(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>>;
auto createSyncObjects(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>>;
auto drawFrame(Vulkan &vulkan) -> tl::expected<void, Error<EngineError>>;
auto reloadShadersAndPipeline(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>>;
void cleanup(Vulkan &vulkan);

// TODO: remove from public api

auto deviceInitialization(Vulkan &vulkan, int width, int height,
                          std::string const &title, GLFWmonitor *monitor,
                          GLFWwindow *share, bool resize)
    -> tl::expected<void, Error<EngineError>>;
auto createSwapchain(Vulkan &vulkan) -> tl::expected<void, Error<EngineError>>;
auto getQueues(Vulkan &vulkan) -> tl::expected<void, Error<EngineError>>;

}  // namespace mpvgl::vlk
