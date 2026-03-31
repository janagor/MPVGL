#pragma once

#include <filesystem>

#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrapDispatch.h>
#include <vulkan/vulkan.h>

#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::vlk {

class GraphicsPipeline {
   public:
    GraphicsPipeline() = default;
    ~GraphicsPipeline() noexcept;

    GraphicsPipeline(GraphicsPipeline const&) = delete;
    auto operator=(GraphicsPipeline const&) -> GraphicsPipeline& = delete;

    GraphicsPipeline(GraphicsPipeline&& other) noexcept;
    auto operator=(GraphicsPipeline&& other) noexcept -> GraphicsPipeline&;

    [[nodiscard]] static auto create(
        DeviceContext const& device, VkRenderPass renderPass,
        VkDescriptorSetLayout descriptorSetLayout,
        std::filesystem::path const& vertexShaderPath,
        std::filesystem::path const& fragmentShaderPath)
        -> tl::expected<GraphicsPipeline, Error<EngineError>>;

    [[nodiscard]] auto handle() const noexcept -> VkPipeline {
        return m_pipeline;
    }
    [[nodiscard]] auto layout() const noexcept -> VkPipelineLayout {
        return m_layout;
    }

   private:
    GraphicsPipeline(VkPipeline pipeline, VkPipelineLayout layout,
                     vkb::DispatchTable disp) noexcept;
    void cleanup() noexcept;

    VkPipeline m_pipeline{VK_NULL_HANDLE};
    VkPipelineLayout m_layout{VK_NULL_HANDLE};
    vkb::DispatchTable m_disp;
};

}  // namespace mpvgl::vlk
