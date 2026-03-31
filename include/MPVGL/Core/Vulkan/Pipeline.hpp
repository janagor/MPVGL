#pragma once

#include <vector>

#include <tl/expected.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::vlk {

class PipelineBuilder {
   public:
    PipelineBuilder();

    auto setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader)
        -> PipelineBuilder&;
    auto setVertexInput(
        std::vector<VkVertexInputBindingDescription> bindings,
        std::vector<VkVertexInputAttributeDescription> attributes)
        -> PipelineBuilder&;
    auto setInputTopology(VkPrimitiveTopology topology) -> PipelineBuilder&;
    auto setPolygonMode(VkPolygonMode mode) -> PipelineBuilder&;
    auto setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
        -> PipelineBuilder&;
    auto setMultisampling(VkSampleCountFlagBits samples) -> PipelineBuilder&;
    auto setDepthStencil(VkBool32 depthTestEnable, VkBool32 depthWriteEnable,
                         VkCompareOp compareOp) -> PipelineBuilder&;
    auto disableColorBlending() -> PipelineBuilder&;
    auto setDynamicStates(std::vector<VkDynamicState> states)
        -> PipelineBuilder&;

    auto build(DeviceContext const& device, VkRenderPass pass,
               VkPipelineLayout layout)
        -> tl::expected<VkPipeline, Error<EngineError>>;

   private:
    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
    std::vector<VkVertexInputBindingDescription> m_vertexBindings;
    std::vector<VkVertexInputAttributeDescription> m_vertexAttributes;
    std::vector<VkDynamicState> m_dynamicStates;

    VkPipelineInputAssemblyStateCreateInfo m_inputAssembly{};
    VkPipelineRasterizationStateCreateInfo m_rasterizer{};
    VkPipelineColorBlendAttachmentState m_colorBlendAttachment{};
    VkPipelineMultisampleStateCreateInfo m_multisampling{};
    VkPipelineDepthStencilStateCreateInfo m_depthStencil{};
};

}  // namespace mpvgl::vlk
