#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>
#include <vector>

#include <tl/expected.hpp>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Error.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Core/Vulkan/Pipeline.hpp"

namespace mpvgl::vlk {

PipelineBuilder::PipelineBuilder() {
    m_inputAssembly = initializers::pipelineInputAssemblyStateCreateInfo(
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
    m_rasterizer = initializers::pipelineRasterizationStateCreateInfo(
        VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    m_multisampling =
        initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
    m_depthStencil = initializers::pipelineDepthStencilStateCreateInfo(
        VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);
    disableColorBlending();
}

PipelineBuilder& PipelineBuilder::setShaders(VkShaderModule vertexShader,
                                             VkShaderModule fragmentShader) {
    m_shaderStages.clear();
    m_shaderStages.push_back(initializers::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_VERTEX_BIT, vertexShader, "main"));
    m_shaderStages.push_back(initializers::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, "main"));
    return *this;
}

PipelineBuilder& PipelineBuilder::setVertexInput(
    std::vector<VkVertexInputBindingDescription> bindings,
    std::vector<VkVertexInputAttributeDescription> attributes) {
    m_vertexBindings = std::move(bindings);
    m_vertexAttributes = std::move(attributes);
    return *this;
}

PipelineBuilder& PipelineBuilder::setInputTopology(
    VkPrimitiveTopology topology) {
    m_inputAssembly.topology = topology;
    return *this;
}

PipelineBuilder& PipelineBuilder::setPolygonMode(VkPolygonMode mode) {
    m_rasterizer.polygonMode = mode;
    return *this;
}

PipelineBuilder& PipelineBuilder::setCullMode(VkCullModeFlags cullMode,
                                              VkFrontFace frontFace) {
    m_rasterizer.cullMode = cullMode;
    m_rasterizer.frontFace = frontFace;
    return *this;
}

PipelineBuilder& PipelineBuilder::setMultisampling(
    VkSampleCountFlagBits samples) {
    m_multisampling.rasterizationSamples = samples;
    return *this;
}

PipelineBuilder& PipelineBuilder::setDepthStencil(VkBool32 depthTestEnable,
                                                  VkBool32 depthWriteEnable,
                                                  VkCompareOp compareOp) {
    m_depthStencil = initializers::pipelineDepthStencilStateCreateInfo(
        depthTestEnable, depthWriteEnable, compareOp);
    m_depthStencil.minDepthBounds = 0.0f;
    m_depthStencil.maxDepthBounds = 1.0f;
    return *this;
}

PipelineBuilder& PipelineBuilder::disableColorBlending() {
    m_colorBlendAttachment = {};
    m_colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    m_colorBlendAttachment.blendEnable = VK_FALSE;
    return *this;
}

PipelineBuilder& PipelineBuilder::setDynamicStates(
    std::vector<VkDynamicState> states) {
    m_dynamicStates = std::move(states);
    return *this;
}

tl::expected<VkPipeline, Error> PipelineBuilder::build(
    DeviceContext const& device, VkRenderPass pass, VkPipelineLayout layout) {
    auto vertexInputInfo = initializers::pipelineVertexInputStateCreateInfo(
        m_vertexBindings, m_vertexAttributes);
    auto viewportState = initializers::pipelineViewportStateCreateInfo(1, 1);

    auto colorBlending = initializers::pipelineColorBlendStateCreateInfo(
        {&m_colorBlendAttachment, 1}, VK_FALSE, VK_LOGIC_OP_COPY);
    auto blendConstants = std::array{0.0f, 0.0f, 0.0f, 0.0f};
    std::copy(blendConstants.begin(), blendConstants.end(),
              colorBlending.blendConstants);

    auto dynamicInfo =
        initializers::pipelineDynamicStateCreateInfo(m_dynamicStates);

    auto pipelineInfo = initializers::graphicsPipelineCreateInfo();
    pipelineInfo.stageCount = static_cast<uint32_t>(m_shaderStages.size());
    pipelineInfo.pStages = m_shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &m_inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &m_rasterizer;
    pipelineInfo.pMultisampleState = &m_multisampling;
    pipelineInfo.pDepthStencilState = &m_depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicInfo;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkPipeline newPipeline;
    if (device.logDevDisp.createGraphicsPipelines(VK_NULL_HANDLE, 1,
                                                  &pipelineInfo, nullptr,
                                                  &newPipeline) != VK_SUCCESS) {
        return tl::unexpected(Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Graphics Pipeline"});
    }
    return newPipeline;
}

}  // namespace mpvgl::vlk
