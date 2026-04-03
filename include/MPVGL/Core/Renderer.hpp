#pragma once

#include <tl/expected.hpp>

#include "MPVGL/Core/Scene.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::vlk {

class Renderer {
   public:
    Renderer() = default;

    auto bootstrap(GLFWwindow *window)
        -> tl::expected<void, Error<EngineError>>;
    auto setupRenderingPipeline() -> tl::expected<void, Error<EngineError>>;
    auto setupRenderTargets() -> tl::expected<void, Error<EngineError>>;
    auto loadAndPrepareAssets(Scene const &scene)
        -> tl::expected<void, Error<EngineError>>;
    auto setupDescriptorsAndSync() -> tl::expected<void, Error<EngineError>>;

    auto createRenderPass() -> tl::expected<void, Error<EngineError>>;
    auto createDescriptorSetLayout() -> tl::expected<void, Error<EngineError>>;
    auto createGraphicsPipeline() -> tl::expected<void, Error<EngineError>>;
    auto createCommandPool() -> tl::expected<void, Error<EngineError>>;
    auto createDepthResources() -> tl::expected<void, Error<EngineError>>;
    auto createFramebuffers() -> tl::expected<void, Error<EngineError>>;
    auto loadScene(Scene const &scene)
        -> tl::expected<void, Error<EngineError>>;
    auto createUniformBuffers() -> tl::expected<void, Error<EngineError>>;
    auto createDescriptorPool() -> tl::expected<void, Error<EngineError>>;
    auto createCommandBuffers() -> tl::expected<void, Error<EngineError>>;
    auto createSyncObjects() -> tl::expected<void, Error<EngineError>>;
    auto drawFrame() -> tl::expected<void, Error<EngineError>>;
    auto reloadShadersAndPipeline() -> tl::expected<void, Error<EngineError>>;
    void cleanup();

    // TODO: remove from public api

    auto deviceInitialization(
        GLFWwindow *window
        // int width, int height, std::string const &title,
        //                       GLFWmonitor *monitor, GLFWwindow *share,
        //                       bool resize
        ) -> tl::expected<void, Error<EngineError>>;
    auto createSwapchain() -> tl::expected<void, Error<EngineError>>;
    auto getQueues() -> tl::expected<void, Error<EngineError>>;

    template <typename Self>
    [[nodiscard]] auto vulkan(this Self &&self) -> auto && {
        return std::forward<Self>(self).m_vulkan;
    }

   private:
    Vulkan m_vulkan{};
};

}  // namespace mpvgl::vlk
