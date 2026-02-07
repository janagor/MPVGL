#pragma once

#include <cinttypes>
#include <span>

#include "vulkan/vulkan.h"

namespace mpvgl::vlk::initializers {

inline VkRenderPassCreateInfo renderPassCreateInfo(
    std::span<const VkAttachmentDescription> attachments,
    std::span<const VkSubpassDescription> subpasses,
    std::span<const VkSubpassDependency> dependencies,
    VkRenderPassCreateFlags flags = 0) {
    return VkRenderPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .attachmentCount = static_cast<std::uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = static_cast<std::uint32_t>(subpasses.size()),
        .pSubpasses = subpasses.data(),
        .dependencyCount = static_cast<std::uint32_t>(dependencies.size()),
        .pDependencies = dependencies.data(),
    };
}

inline VkShaderModuleCreateInfo shaderModuleCreateInfo(
    std::span<const std::uint32_t> code) {
    return VkShaderModuleCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = static_cast<std::size_t>(code.size_bytes()),
        .pCode = code.data(),
    };
}

inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
    std::span<const VkDescriptorSetLayoutBinding> bindings,
    VkDescriptorSetLayoutCreateFlags flags = 0) {
    return VkDescriptorSetLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .bindingCount = static_cast<std::uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };
}

inline VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(
    VkShaderStageFlagBits stage, VkShaderModule module, char const* pName,
    VkSpecializationInfo const* pSpecializationInfo = nullptr,
    VkPipelineShaderStageCreateFlags flags = 0) {
    return VkPipelineShaderStageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .stage = stage,
        .module = module,
        .pName = pName,
        .pSpecializationInfo = pSpecializationInfo,
    };
}

inline VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo(
    std::span<const VkVertexInputBindingDescription> bindingDescriptions,
    std::span<const VkVertexInputAttributeDescription> attributeDescriptions) {
    return VkPipelineVertexInputStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount =
            static_cast<std::uint32_t>(bindingDescriptions.size()),
        .pVertexBindingDescriptions = bindingDescriptions.data(),
        .vertexAttributeDescriptionCount =
            static_cast<std::uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data(),
    };
}

inline VkPipelineInputAssemblyStateCreateInfo
pipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology,
                                     VkBool32 primitiveRestartEnable) {
    return VkPipelineInputAssemblyStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = topology,
        .primitiveRestartEnable = primitiveRestartEnable,
    };
}

inline VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(
    std::uint32_t viewportCount, std::uint32_t scissorCount) {
    return VkPipelineViewportStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = viewportCount,
        .pViewports = nullptr,
        .scissorCount = scissorCount,
        .pScissors = nullptr,
    };
}

inline VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(
    std::span<const VkViewport> viewports, std::span<const VkRect2D> scissors) {
    return VkPipelineViewportStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = static_cast<std::uint32_t>(viewports.size()),
        .pViewports = viewports.data(),
        .scissorCount = static_cast<std::uint32_t>(scissors.size()),
        .pScissors = scissors.data(),
    };
}

inline VkPipelineRasterizationStateCreateInfo
pipelineRasterizationStateCreateInfo(VkPolygonMode polygonMode,
                                     VkCullModeFlags cullMode,
                                     VkFrontFace frontFace) {
    return VkPipelineRasterizationStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = polygonMode,
        .cullMode = cullMode,
        .frontFace = frontFace,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };
}

inline VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(
    VkSampleCountFlagBits rasterizationSamples) {
    return VkPipelineMultisampleStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = rasterizationSamples,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = false,
        .alphaToOneEnable = false,
    };
}

inline VkPipelineDepthStencilStateCreateInfo
pipelineDepthStencilStateCreateInfo(
    VkBool32 depthTestEnable, VkBool32 depthWriteEnable,
    VkCompareOp depthCompareOp,
    VkPipelineDepthStencilStateCreateFlags flags = 0) {
    return VkPipelineDepthStencilStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .depthTestEnable = depthTestEnable,
        .depthWriteEnable = depthWriteEnable,
        .depthCompareOp = depthCompareOp,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 0.0f,
    };
}

inline VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
    std::span<const VkPipelineColorBlendAttachmentState> states,
    VkBool32 logicOpEnable, VkLogicOp logicOp,
    VkPipelineColorBlendStateCreateFlags flags = 0) {
    return VkPipelineColorBlendStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .logicOpEnable = logicOpEnable,
        .logicOp = logicOp,
        .attachmentCount = static_cast<std::uint32_t>(states.size()),
        .pAttachments = states.data(),
    };
}

inline VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
    std::span<const VkDynamicState> states) {
    return VkPipelineDynamicStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = static_cast<std::uint32_t>(states.size()),
        .pDynamicStates = states.data(),
    };
}

inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
    std::span<const VkDescriptorSetLayout> setLayouts,
    std::span<const VkPushConstantRange> pushConstantRanges,
    VkPipelineLayoutCreateFlags flags = 0) {
    return VkPipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .flags = flags,
        .setLayoutCount = static_cast<std::uint32_t>(setLayouts.size()),
        .pSetLayouts = setLayouts.data(),
        .pushConstantRangeCount =
            static_cast<std::uint32_t>(pushConstantRanges.size()),
        .pPushConstantRanges = pushConstantRanges.data(),
    };
}

inline VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo() {
    return VkGraphicsPipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
    };
}

inline VkFramebufferCreateInfo framebufferCreateInfo(
    VkRenderPass renderPass, std::span<const VkImageView> attachments,
    VkExtent2D const& extent2D, uint32_t layers,
    VkFramebufferCreateFlags flags = 0) {
    return VkFramebufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .renderPass = renderPass,
        .attachmentCount = static_cast<std::uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = extent2D.width,
        .height = extent2D.height,
        .layers = layers,
    };
}

inline VkCommandPoolCreateInfo commandPoolCreateInfo(
    std::uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) {
    return VkCommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .queueFamilyIndex = queueFamilyIndex,
    };
}

inline VkMemoryAllocateInfo memoryAllocateInfo(VkDeviceSize allocationSize,
                                               std::uint32_t memoryTypeIndex) {
    return VkMemoryAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = allocationSize,
        .memoryTypeIndex = memoryTypeIndex,
    };
}

inline VkWriteDescriptorSet writeDescriptorSet(
    VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t dstArrayElement,
    uint32_t descriptorCount, VkDescriptorType descriptorType,
    VkDescriptorImageInfo const* pImageInfo,
    VkDescriptorBufferInfo const* pBufferInfo,
    VkBufferView const* pTexelBufferView) {
    return VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = dstSet,
        .dstBinding = dstBinding,
        .dstArrayElement = dstArrayElement,
        .descriptorCount = descriptorCount,
        .descriptorType = descriptorType,
        .pImageInfo = pImageInfo,
        .pBufferInfo = pBufferInfo,
        .pTexelBufferView = pTexelBufferView,

    };
}

inline VkWriteDescriptorSet writeDescriptorSet(
    VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t dstArrayElement,
    VkDescriptorType descriptorType,
    std::span<const VkDescriptorImageInfo> descriptorImageInfos) {
    return writeDescriptorSet(
        dstSet, dstBinding, dstArrayElement,
        static_cast<std::uint32_t>(descriptorImageInfos.size()), descriptorType,
        descriptorImageInfos.data(), nullptr, nullptr);
}

inline VkWriteDescriptorSet writeDescriptorSet(
    VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t dstArrayElement,
    VkDescriptorType descriptorType,
    std::span<const VkDescriptorBufferInfo> descriptorBufferInfos) {
    return writeDescriptorSet(
        dstSet, dstBinding, dstArrayElement,
        static_cast<std::uint32_t>(descriptorBufferInfos.size()),
        descriptorType, nullptr, descriptorBufferInfos.data(), nullptr);
}

inline VkWriteDescriptorSet writeDescriptorSet(
    VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t dstArrayElement,
    VkDescriptorType descriptorType,
    std::span<const VkBufferView> texelBufferViews) {
    return writeDescriptorSet(
        dstSet, dstBinding, dstArrayElement,
        static_cast<std::uint32_t>(texelBufferViews.size()), descriptorType,
        nullptr, nullptr, texelBufferViews.data());
}

inline VkRenderPassBeginInfo renderPassBeginInfo(
    VkRenderPass renderPass, VkFramebuffer framebuffer,
    VkRect2D const& renderArea, std::span<const VkClearValue> clearValues) {
    return VkRenderPassBeginInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = renderPass,
        .framebuffer = framebuffer,
        .renderArea = renderArea,
        .clearValueCount = static_cast<std::uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
    };
}

inline VkBufferCreateInfo bufferCreateInfo(VkDeviceSize size,
                                           VkBufferUsageFlags usage,
                                           VkBufferCreateFlags flags = 0) {
    return VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
}

inline VkImageCreateInfo imageCreateInfo() {
    return VkImageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    };
}

inline VkImageMemoryBarrier imageMemoryBarrier(
    VkImageLayout oldLayout, VkImageLayout newLayout, VkImage image,
    VkImageSubresourceRange subresourceRange) {
    return VkImageMemoryBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_NONE,
        .dstAccessMask = VK_ACCESS_NONE,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = subresourceRange,
    };
}

inline VkImageViewCreateInfo imageViewCreateInfo(
    VkImage image, VkImageViewType viewType, VkFormat format,
    VkImageSubresourceRange subresourceRange, VkImageViewCreateFlags flags = 0,
    VkComponentMapping components = VkComponentMapping{
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    }) {
    return VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = image,
        .viewType = viewType,
        .format = format,
        .components = components,
        .subresourceRange = subresourceRange,
    };
}

inline VkSamplerCreateInfo samplerCreateInfo() {
    return VkSamplerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    };
}

inline VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
    std::span<const VkDescriptorPoolSize> sizes, std::uint32_t maxSets) {
    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = static_cast<std::uint32_t>(sizes.size());
    info.pPoolSizes = sizes.data();
    info.maxSets = maxSets;
    return info;
}

inline VkDescriptorSetAllocateInfo descriptorSetAllocateInfo(
    VkDescriptorPool descriptorPool,
    std::span<const VkDescriptorSetLayout> descriptorSetLayouts) {
    return VkDescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = descriptorPool,
        .descriptorSetCount =
            static_cast<std::uint32_t>(descriptorSetLayouts.size()),
        .pSetLayouts = descriptorSetLayouts.data(),
    };
}

inline VkCommandBufferAllocateInfo commandBufferAllocateInfo(
    VkCommandPool commandPool, VkCommandBufferLevel level,
    std::uint32_t commandBufferCount) {
    return VkCommandBufferAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = commandPool,
        .level = level,
        .commandBufferCount = commandBufferCount,
    };
}

inline VkSemaphoreCreateInfo semaphoreCreateInfo() {
    return VkSemaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };
}
inline VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0) {
    return VkFenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
    };
}

inline VkCommandBufferBeginInfo commandBufferBeginInfo(
    VkCommandBufferUsageFlags flags = 0) {
    return VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = flags,
        .pInheritanceInfo = nullptr,
    };
}

inline VkSubmitInfo submitInfo(
    std::span<const VkSemaphore> waitSemaphores,
    std::span<const VkPipelineStageFlags> waitDstStageMask,
    std::span<const VkCommandBuffer> commandBuffers,
    std::span<const VkSemaphore> signalSemaphores) {
    return VkSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = static_cast<std::uint32_t>(waitSemaphores.size()),
        .pWaitSemaphores = waitSemaphores.data(),
        .pWaitDstStageMask = waitDstStageMask.data(),
        .commandBufferCount = static_cast<std::uint32_t>(commandBuffers.size()),
        .pCommandBuffers = commandBuffers.data(),
        .signalSemaphoreCount =
            static_cast<std::uint32_t>(signalSemaphores.size()),
        .pSignalSemaphores = signalSemaphores.data(),
    };
}

inline VkPresentInfoKHR presentInfoKHR(
    std::span<const VkSemaphore> waitSemaphores,
    std::span<const VkSwapchainKHR> swapchains,
    std::span<const std::uint32_t> indices) {
    return VkPresentInfoKHR{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = static_cast<std::uint32_t>(waitSemaphores.size()),
        .pWaitSemaphores = waitSemaphores.data(),
        .swapchainCount = static_cast<std::uint32_t>(swapchains.size()),
        .pSwapchains = swapchains.data(),
        .pImageIndices = indices.data(),
        .pResults = nullptr,
    };
}

}  // namespace mpvgl::vlk::initializers
