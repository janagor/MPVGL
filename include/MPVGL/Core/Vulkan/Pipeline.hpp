#pragma once

#include <vector>

#include <tl/expected.hpp>
#include <vulkan/vulkan.h>

#include "MPVGL/Core/Error.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"

namespace mpvgl::vlk {

class PipelineBuilder {
   public:
    PipelineBuilder();

    PipelineBuilder& setShaders(VkShaderModule vertexShader,
                                VkShaderModule fragmentShader);
    PipelineBuilder& setVertexInput(
        std::vector<VkVertexInputBindingDescription> bindings,
        std::vector<VkVertexInputAttributeDescription> attributes);
    PipelineBuilder& setInputTopology(VkPrimitiveTopology topology);
    PipelineBuilder& setPolygonMode(VkPolygonMode mode);
    PipelineBuilder& setCullMode(VkCullModeFlags cullMode,
                                 VkFrontFace frontFace);
    PipelineBuilder& setMultisampling(VkSampleCountFlagBits samples);
    PipelineBuilder& setDepthStencil(VkBool32 depthTestEnable,
                                     VkBool32 depthWriteEnable,
                                     VkCompareOp compareOp);
    PipelineBuilder& disableColorBlending();
    PipelineBuilder& setDynamicStates(std::vector<VkDynamicState> states);

    tl::expected<VkPipeline, Error> build(DeviceContext const& device,
                                          VkRenderPass pass,
                                          VkPipelineLayout layout);

   private:
    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages{};
    std::vector<VkVertexInputBindingDescription> m_vertexBindings{};
    std::vector<VkVertexInputAttributeDescription> m_vertexAttributes{};
    std::vector<VkDynamicState> m_dynamicStates{};

    VkPipelineInputAssemblyStateCreateInfo m_inputAssembly{};
    VkPipelineRasterizationStateCreateInfo m_rasterizer{};
    VkPipelineColorBlendAttachmentState m_colorBlendAttachment{};
    VkPipelineMultisampleStateCreateInfo m_multisampling{};
    VkPipelineDepthStencilStateCreateInfo m_depthStencil{};
};

}  // namespace mpvgl::vlk
