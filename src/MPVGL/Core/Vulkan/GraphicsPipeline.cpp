#include <filesystem>

#include <glm/ext/matrix_float4x4.hpp>
#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrapDispatch.h>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Core/Vulkan/GraphicsPipeline.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Core/Vulkan/Internal.hpp"
#include "MPVGL/Core/Vulkan/Pipeline.hpp"
#include "MPVGL/Core/Vulkan/Vertex.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"
#include "MPVGL/IO/ResourceBuffer.hpp"

namespace mpvgl::vlk {

GraphicsPipeline::GraphicsPipeline(VkPipeline pipeline, VkPipelineLayout layout,
                                   vkb::DispatchTable disp) noexcept
    : m_pipeline(pipeline), m_layout(layout), m_disp(disp) {}

GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& other) noexcept
    : m_pipeline(other.m_pipeline),
      m_layout(other.m_layout),
      m_disp(other.m_disp) {
    other.m_pipeline = VK_NULL_HANDLE;
    other.m_layout = VK_NULL_HANDLE;
}

GraphicsPipeline& GraphicsPipeline::operator=(
    GraphicsPipeline&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_pipeline = other.m_pipeline;
        m_layout = other.m_layout;
        m_disp = other.m_disp;

        other.m_pipeline = VK_NULL_HANDLE;
        other.m_layout = VK_NULL_HANDLE;
    }
    return *this;
}

GraphicsPipeline::~GraphicsPipeline() noexcept { cleanup(); }

void GraphicsPipeline::cleanup() noexcept {
    if (m_pipeline != VK_NULL_HANDLE) {
        m_disp.destroyPipeline(m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    if (m_layout != VK_NULL_HANDLE) {
        m_disp.destroyPipelineLayout(m_layout, nullptr);
        m_layout = VK_NULL_HANDLE;
    }
}

tl::expected<GraphicsPipeline, Error<EngineError>> GraphicsPipeline::create(
    DeviceContext const& device, VkRenderPass renderPass,
    VkDescriptorSetLayout descriptorSetLayout,
    std::filesystem::path const& vertexShaderPath,
    std::filesystem::path const& fragmentShaderPath) {
    auto vertBufferRes =
        io::ResourceBuffer::load(vertexShaderPath).map_error([](auto const& e) {
            return Error<EngineError>{EngineError::VulkanRuntimeError,
                                      e.message()};
        });
    if (!vertBufferRes) return tl::unexpected{vertBufferRes.error()};

    auto fragBufferRes =
        io::ResourceBuffer::load(fragmentShaderPath)
            .map_error([](auto const& e) {
                return Error<EngineError>{EngineError::VulkanRuntimeError,
                                          e.message()};
            });
    if (!fragBufferRes) return tl::unexpected{fragBufferRes.error()};

    auto vertModule = createShaderModule(device, vertBufferRes->view());
    auto fragModule = createShaderModule(device, fragBufferRes->view());

    if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE) {
        if (vertModule != VK_NULL_HANDLE)
            device.logDevDisp.destroyShaderModule(vertModule, nullptr);
        if (fragModule != VK_NULL_HANDLE)
            device.logDevDisp.destroyShaderModule(fragModule, nullptr);
        return tl::unexpected{
            Error{EngineError::ShaderError, "Failed to create Shader Modules"}};
    }

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);

    auto pipelineLayoutInfo = initializers::pipelineLayoutCreateInfo(
        {&descriptorSetLayout, 1}, {&pushConstantRange, 1});
    VkPipelineLayout pipelineLayout;

    if (device.logDevDisp.createPipelineLayout(&pipelineLayoutInfo, nullptr,
                                               &pipelineLayout) != VK_SUCCESS) {
        device.logDevDisp.destroyShaderModule(fragModule, nullptr);
        device.logDevDisp.destroyShaderModule(vertModule, nullptr);
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Pipeline Layout"}};
    }

    PipelineBuilder builder{};
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    auto pipelineRes =
        builder.setShaders(vertModule, fragModule)
            .setVertexInput(
                {bindingDescription},
                {attributeDescriptions.begin(), attributeDescriptions.end()})
            .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .setPolygonMode(VK_POLYGON_MODE_FILL)
            .setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .setMultisampling(VK_SAMPLE_COUNT_1_BIT)
            .setDepthStencil(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS)
            .disableColorBlending()
            .setDynamicStates(
                {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR})
            .build(device, renderPass, pipelineLayout);

    device.logDevDisp.destroyShaderModule(fragModule, nullptr);
    device.logDevDisp.destroyShaderModule(vertModule, nullptr);

    if (!pipelineRes) {
        device.logDevDisp.destroyPipelineLayout(pipelineLayout, nullptr);
        return tl::unexpected{pipelineRes.error()};
    }

    return GraphicsPipeline(pipelineRes.value(), pipelineLayout,
                            device.logDevDisp);
}

}  // namespace mpvgl::vlk
