#pragma once

#include <string>

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

    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    GraphicsPipeline(GraphicsPipeline&& other) noexcept;
    GraphicsPipeline& operator=(GraphicsPipeline&& other) noexcept;

    [[nodiscard]] static tl::expected<GraphicsPipeline, Error<EngineError>>
    create(DeviceContext const& device, VkRenderPass renderPass,
           VkDescriptorSetLayout descriptorSetLayout,
           std::string const& vertexShaderPath,
           std::string const& fragmentShaderPath);

    [[nodiscard]] VkPipeline handle() const noexcept { return m_pipeline; }
    [[nodiscard]] VkPipelineLayout layout() const noexcept { return m_layout; }

   private:
    GraphicsPipeline(VkPipeline pipeline, VkPipelineLayout layout,
                     vkb::DispatchTable disp) noexcept;
    void cleanup() noexcept;

   private:
    VkPipeline m_pipeline{VK_NULL_HANDLE};
    VkPipelineLayout m_layout{VK_NULL_HANDLE};
    vkb::DispatchTable m_disp{};
};

}  // namespace mpvgl::vlk
