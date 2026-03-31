#pragma once

#include <cstddef>
#include <span>

#include <vulkan/vulkan_core.h>

#include "MPVGL/Utility/Types.hpp"

namespace mpvgl::vlk::initializers {

inline auto renderPassCreateInfo(
    std::span<VkAttachmentDescription const> attachments,
    std::span<VkSubpassDescription const> subpasses,
    std::span<VkSubpassDependency const> dependencies,
    VkRenderPassCreateFlags flags = 0) -> VkRenderPassCreateInfo {
    return VkRenderPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .attachmentCount = static_cast<u32>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = static_cast<u32>(subpasses.size()),
        .pSubpasses = subpasses.data(),
        .dependencyCount = static_cast<u32>(dependencies.size()),
        .pDependencies = dependencies.data(),
    };
}

inline auto shaderModuleCreateInfo(std::span<u32 const> code)
    -> VkShaderModuleCreateInfo {
    return VkShaderModuleCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = static_cast<std::size_t>(code.size_bytes()),
        .pCode = code.data(),
    };
}

inline auto descriptorSetLayoutCreateInfo(
    std::span<VkDescriptorSetLayoutBinding const> bindings,
    VkDescriptorSetLayoutCreateFlags flags = 0)
    -> VkDescriptorSetLayoutCreateInfo {
    return VkDescriptorSetLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .bindingCount = static_cast<u32>(bindings.size()),
        .pBindings = bindings.data(),
    };
}

inline auto pipelineShaderStageCreateInfo(
    VkShaderStageFlagBits stage, VkShaderModule module, char const* pName,
    VkSpecializationInfo const* pSpecializationInfo = nullptr,
    VkPipelineShaderStageCreateFlags flags = 0)
    -> VkPipelineShaderStageCreateInfo {
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

inline auto pipelineVertexInputStateCreateInfo(
    std::span<VkVertexInputBindingDescription const> bindingDescriptions,
    std::span<VkVertexInputAttributeDescription const> attributeDescriptions)
    -> VkPipelineVertexInputStateCreateInfo {
    return VkPipelineVertexInputStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount =
            static_cast<u32>(bindingDescriptions.size()),
        .pVertexBindingDescriptions = bindingDescriptions.data(),
        .vertexAttributeDescriptionCount =
            static_cast<u32>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data(),
    };
}

inline auto pipelineInputAssemblyStateCreateInfo(
    VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable)
    -> VkPipelineInputAssemblyStateCreateInfo {
    return VkPipelineInputAssemblyStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = topology,
        .primitiveRestartEnable = primitiveRestartEnable,
    };
}

inline auto pipelineViewportStateCreateInfo(u32 viewportCount, u32 scissorCount)
    -> VkPipelineViewportStateCreateInfo {
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

inline auto pipelineViewportStateCreateInfo(
    std::span<VkViewport const> viewports, std::span<VkRect2D const> scissors)
    -> VkPipelineViewportStateCreateInfo {
    return VkPipelineViewportStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = static_cast<u32>(viewports.size()),
        .pViewports = viewports.data(),
        .scissorCount = static_cast<u32>(scissors.size()),
        .pScissors = scissors.data(),
    };
}

inline auto pipelineRasterizationStateCreateInfo(VkPolygonMode polygonMode,
                                                 VkCullModeFlags cullMode,
                                                 VkFrontFace frontFace)
    -> VkPipelineRasterizationStateCreateInfo {
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
        .depthBiasConstantFactor = 0.0F,
        .depthBiasClamp = 0.0F,
        .depthBiasSlopeFactor = 0.0F,
        .lineWidth = 1.0F,
    };
}

inline auto pipelineMultisampleStateCreateInfo(
    VkSampleCountFlagBits rasterizationSamples)
    -> VkPipelineMultisampleStateCreateInfo {
    return VkPipelineMultisampleStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = rasterizationSamples,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0F,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = 0U,
        .alphaToOneEnable = 0U,
    };
}

inline auto pipelineDepthStencilStateCreateInfo(
    VkBool32 depthTestEnable, VkBool32 depthWriteEnable,
    VkCompareOp depthCompareOp,
    VkPipelineDepthStencilStateCreateFlags flags = 0)
    -> VkPipelineDepthStencilStateCreateInfo {
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
        .minDepthBounds = 0.0F,
        .maxDepthBounds = 0.0F,
    };
}

inline auto pipelineColorBlendStateCreateInfo(
    std::span<VkPipelineColorBlendAttachmentState const> states,
    VkBool32 logicOpEnable, VkLogicOp logicOp,
    VkPipelineColorBlendStateCreateFlags flags = 0)
    -> VkPipelineColorBlendStateCreateInfo {
    return VkPipelineColorBlendStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .logicOpEnable = logicOpEnable,
        .logicOp = logicOp,
        .attachmentCount = static_cast<u32>(states.size()),
        .pAttachments = states.data(),
    };
}

inline auto pipelineDynamicStateCreateInfo(
    std::span<VkDynamicState const> states)
    -> VkPipelineDynamicStateCreateInfo {
    return VkPipelineDynamicStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = static_cast<u32>(states.size()),
        .pDynamicStates = states.data(),
    };
}

inline auto pipelineLayoutCreateInfo(
    std::span<VkDescriptorSetLayout const> setLayouts,
    std::span<VkPushConstantRange const> pushConstantRanges,
    VkPipelineLayoutCreateFlags flags = 0) -> VkPipelineLayoutCreateInfo {
    return VkPipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .flags = flags,
        .setLayoutCount = static_cast<u32>(setLayouts.size()),
        .pSetLayouts = setLayouts.data(),
        .pushConstantRangeCount = static_cast<u32>(pushConstantRanges.size()),
        .pPushConstantRanges = pushConstantRanges.data(),
    };
}

inline auto graphicsPipelineCreateInfo() -> VkGraphicsPipelineCreateInfo {
    return VkGraphicsPipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
    };
}

inline auto framebufferCreateInfo(VkRenderPass renderPass,
                                  std::span<VkImageView const> attachments,
                                  VkExtent2D const& extent2D, u32 layers,
                                  VkFramebufferCreateFlags flags = 0)
    -> VkFramebufferCreateInfo {
    return VkFramebufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .renderPass = renderPass,
        .attachmentCount = static_cast<u32>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = extent2D.width,
        .height = extent2D.height,
        .layers = layers,
    };
}

inline auto commandPoolCreateInfo(u32 queueFamilyIndex,
                                  VkCommandPoolCreateFlags flags = 0)
    -> VkCommandPoolCreateInfo {
    return VkCommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .queueFamilyIndex = queueFamilyIndex,
    };
}

inline auto memoryAllocateInfo(VkDeviceSize allocationSize, u32 memoryTypeIndex)
    -> VkMemoryAllocateInfo {
    return VkMemoryAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = allocationSize,
        .memoryTypeIndex = memoryTypeIndex,
    };
}

inline auto writeDescriptorSet(VkDescriptorSet dstSet, u32 dstBinding,
                               u32 dstArrayElement, u32 descriptorCount,
                               VkDescriptorType descriptorType,
                               VkDescriptorImageInfo const* pImageInfo,
                               VkDescriptorBufferInfo const* pBufferInfo,
                               VkBufferView const* pTexelBufferView)
    -> VkWriteDescriptorSet {
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

inline auto writeDescriptorSet(
    VkDescriptorSet dstSet, u32 dstBinding, u32 dstArrayElement,
    VkDescriptorType descriptorType,
    std::span<VkDescriptorImageInfo const> descriptorImageInfos)
    -> VkWriteDescriptorSet {
    return writeDescriptorSet(dstSet, dstBinding, dstArrayElement,
                              static_cast<u32>(descriptorImageInfos.size()),
                              descriptorType, descriptorImageInfos.data(),
                              nullptr, nullptr);
}

inline auto writeDescriptorSet(
    VkDescriptorSet dstSet, u32 dstBinding, u32 dstArrayElement,
    VkDescriptorType descriptorType,
    std::span<VkDescriptorBufferInfo const> descriptorBufferInfos)
    -> VkWriteDescriptorSet {
    return writeDescriptorSet(dstSet, dstBinding, dstArrayElement,
                              static_cast<u32>(descriptorBufferInfos.size()),
                              descriptorType, nullptr,
                              descriptorBufferInfos.data(), nullptr);
}

inline auto writeDescriptorSet(VkDescriptorSet dstSet, u32 dstBinding,
                               u32 dstArrayElement,
                               VkDescriptorType descriptorType,
                               std::span<VkBufferView const> texelBufferViews)
    -> VkWriteDescriptorSet {
    return writeDescriptorSet(dstSet, dstBinding, dstArrayElement,
                              static_cast<u32>(texelBufferViews.size()),
                              descriptorType, nullptr, nullptr,
                              texelBufferViews.data());
}

inline auto renderPassBeginInfo(VkRenderPass renderPass,
                                VkFramebuffer framebuffer,
                                VkRect2D const& renderArea,
                                std::span<VkClearValue const> clearValues)
    -> VkRenderPassBeginInfo {
    return VkRenderPassBeginInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = renderPass,
        .framebuffer = framebuffer,
        .renderArea = renderArea,
        .clearValueCount = static_cast<u32>(clearValues.size()),
        .pClearValues = clearValues.data(),
    };
}

inline auto bufferCreateInfo(VkDeviceSize size, VkBufferUsageFlags usage,
                             VkBufferCreateFlags flags = 0)
    -> VkBufferCreateInfo {
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

inline auto imageCreateInfo() -> VkImageCreateInfo {
    return VkImageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    };
}

inline auto imageMemoryBarrier(VkImageLayout oldLayout, VkImageLayout newLayout,
                               VkImage image,
                               VkImageSubresourceRange subresourceRange)
    -> VkImageMemoryBarrier {
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

inline auto imageViewCreateInfo(
    VkImage image, VkImageViewType viewType, VkFormat format,
    VkImageSubresourceRange subresourceRange, VkImageViewCreateFlags flags = 0,
    VkComponentMapping components = VkComponentMapping{
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    }) -> VkImageViewCreateInfo {
    return VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .image = image,
        .viewType = viewType,
        .format = format,
        .components = components,
        .subresourceRange = subresourceRange,
    };
}

inline auto samplerCreateInfo() -> VkSamplerCreateInfo {
    return VkSamplerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    };
}

inline auto descriptorPoolCreateInfo(
    std::span<VkDescriptorPoolSize const> sizes, u32 maxSets)
    -> VkDescriptorPoolCreateInfo {
    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = static_cast<u32>(sizes.size());
    info.pPoolSizes = sizes.data();
    info.maxSets = maxSets;
    return info;
}

inline auto descriptorSetAllocateInfo(
    VkDescriptorPool descriptorPool,
    std::span<VkDescriptorSetLayout const> descriptorSetLayouts)
    -> VkDescriptorSetAllocateInfo {
    return VkDescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = static_cast<u32>(descriptorSetLayouts.size()),
        .pSetLayouts = descriptorSetLayouts.data(),
    };
}

inline auto commandBufferAllocateInfo(VkCommandPool commandPool,
                                      VkCommandBufferLevel level,
                                      u32 commandBufferCount)
    -> VkCommandBufferAllocateInfo {
    return VkCommandBufferAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = commandPool,
        .level = level,
        .commandBufferCount = commandBufferCount,
    };
}

inline auto semaphoreCreateInfo() -> VkSemaphoreCreateInfo {
    return VkSemaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };
}
inline auto fenceCreateInfo(VkFenceCreateFlags flags = 0) -> VkFenceCreateInfo {
    return VkFenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
    };
}

inline auto commandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0)
    -> VkCommandBufferBeginInfo {
    return VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = flags,
        .pInheritanceInfo = nullptr,
    };
}

inline auto submitInfo(std::span<VkSemaphore const> waitSemaphores,
                       std::span<VkPipelineStageFlags const> waitDstStageMask,
                       std::span<VkCommandBuffer const> commandBuffers,
                       std::span<VkSemaphore const> signalSemaphores)
    -> VkSubmitInfo {
    return VkSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = static_cast<u32>(waitSemaphores.size()),
        .pWaitSemaphores = waitSemaphores.data(),
        .pWaitDstStageMask = waitDstStageMask.data(),
        .commandBufferCount = static_cast<u32>(commandBuffers.size()),
        .pCommandBuffers = commandBuffers.data(),
        .signalSemaphoreCount = static_cast<u32>(signalSemaphores.size()),
        .pSignalSemaphores = signalSemaphores.data(),
    };
}

inline auto presentInfoKHR(std::span<VkSemaphore const> waitSemaphores,
                           std::span<VkSwapchainKHR const> swapchains,
                           std::span<u32 const> indices) -> VkPresentInfoKHR {
    return VkPresentInfoKHR{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = static_cast<u32>(waitSemaphores.size()),
        .pWaitSemaphores = waitSemaphores.data(),
        .swapchainCount = static_cast<u32>(swapchains.size()),
        .pSwapchains = swapchains.data(),
        .pImageIndices = indices.data(),
        .pResults = nullptr,
    };
}

}  // namespace mpvgl::vlk::initializers
