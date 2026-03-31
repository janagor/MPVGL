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
    GraphicsPipeline& operator=(GraphicsPipeline const&) = delete;

    GraphicsPipeline(GraphicsPipeline&& other) noexcept;
    GraphicsPipeline& operator=(GraphicsPipeline&& other) noexcept;

    [[nodiscard]] static tl::expected<GraphicsPipeline, Error<EngineError>>
    create(DeviceContext const& device, VkRenderPass renderPass,
           VkDescriptorSetLayout descriptorSetLayout,
           std::filesystem::path const& vertexShaderPath,
           std::filesystem::path const& fragmentShaderPath);

    [[nodiscard]] VkPipeline handle() const noexcept { return m_pipeline; }
    [[nodiscard]] VkPipelineLayout layout() const noexcept { return m_layout; }

   private:
    GraphicsPipeline(VkPipeline pipeline, VkPipelineLayout layout,
                     vkb::DispatchTable disp) noexcept;
    void cleanup() noexcept;

    VkPipeline m_pipeline{VK_NULL_HANDLE};
    VkPipelineLayout m_layout{VK_NULL_HANDLE};
    vkb::DispatchTable m_disp;
};

}  // namespace mpvgl::vlk
