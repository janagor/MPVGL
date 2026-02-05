#pragma once

#include <span>

#include "vulkan/vulkan.h"

namespace mpvgl::wk::initializers {

inline VkAttachmentDescription attachmentDescription(
    VkAttachmentDescriptionFlags flags, VkFormat format,
    VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp,
    VkAttachmentStoreOp storeOp, VkAttachmentLoadOp stencilLoadOp,
    VkAttachmentStoreOp stencilStoreOp, VkImageLayout initialLayout,
    VkImageLayout finalLayout) {
  VkAttachmentDescription description = {};
  description.format = format;
  description.samples = samples;
  description.loadOp = loadOp;
  description.storeOp = storeOp;
  description.stencilLoadOp = stencilLoadOp;
  description.stencilStoreOp = stencilStoreOp;
  description.initialLayout = initialLayout;
  description.finalLayout = finalLayout;
  return description;
}

inline VkAttachmentReference attachmentReference(uint32_t attachment,
                                                 VkImageLayout layout) {
  VkAttachmentReference reference = {};
  reference.attachment = attachment;
  reference.layout = layout;
  return reference;
}

inline VkRenderPassCreateInfo renderPassCreateInfo(
    VkRenderPassCreateFlags flags,
    std::span<const VkAttachmentDescription> attachments,
    std::span<const VkSubpassDescription> subpasses,
    std::span<const VkSubpassDependency> dependencies) {
  VkRenderPassCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = static_cast<uint32_t>(attachments.size());
  info.pAttachments = attachments.data();
  info.subpassCount = static_cast<uint32_t>(attachments.size());
  info.pSubpasses = subpasses.data();
  info.dependencyCount = static_cast<uint32_t>(attachments.size());
  info.pDependencies = dependencies.data();
  return info;
}

inline VkShaderModuleCreateInfo shaderModuleCreateInfo(
    VkShaderModuleCreateFlags flags, std::span<const uint32_t> code) {
  VkShaderModuleCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.flags = flags;
  info.codeSize = static_cast<std::size_t>(code.size_bytes());
  info.pCode = code.data();
  return info;
}

inline VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
    uint32_t binding, VkDescriptorType type, uint32_t count,
    VkShaderStageFlags flags) {
  VkDescriptorSetLayoutBinding bind{};
  bind.binding = binding;
  bind.descriptorType = type;
  bind.descriptorCount = count;
  bind.stageFlags = flags;
  bind.pImmutableSamplers = nullptr;
  return bind;
}

inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
    VkDescriptorSetLayoutCreateFlags flags,
    std::span<const VkDescriptorSetLayoutBinding> bindings) {
  VkDescriptorSetLayoutCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  info.bindingCount = static_cast<uint32_t>(bindings.size());
  info.pBindings = bindings.data();
  return info;
}

inline VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(
    VkPipelineShaderStageCreateFlags flags, VkShaderStageFlagBits stage,
    VkShaderModule module, const char* pName) {
  VkPipelineShaderStageCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info.flags = flags;
  info.stage = stage;
  info.module = module;
  info.pName = pName;
  return info;
}

inline VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo(
    VkPipelineVertexInputStateCreateFlags flags,
    std::span<const VkVertexInputBindingDescription> bindingDescriptions,
    std::span<const VkVertexInputAttributeDescription> attributeDescriptions) {
  VkPipelineVertexInputStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  info.flags = flags;
  info.vertexBindingDescriptionCount =
      static_cast<uint32_t>(bindingDescriptions.size());
  info.pVertexBindingDescriptions = bindingDescriptions.data();
  info.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  info.pVertexAttributeDescriptions = attributeDescriptions.data();
  return info;
}

inline VkPipelineInputAssemblyStateCreateInfo
pipelineInputAssemblyStateCreateInfo() {
  VkPipelineInputAssemblyStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  info.primitiveRestartEnable = VK_FALSE;
  return info;
}

inline VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo() {
  VkPipelineViewportStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  info.viewportCount = 1;
  info.scissorCount = 1;
  return info;
}

inline VkPipelineRasterizationStateCreateInfo
pipelineRasterizationStateCreateInfo() {
  VkPipelineRasterizationStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  info.depthClampEnable = VK_FALSE;
  info.rasterizerDiscardEnable = VK_FALSE;
  info.polygonMode = VK_POLYGON_MODE_FILL;
  info.lineWidth = 1.0f;
  info.cullMode = VK_CULL_MODE_BACK_BIT;
  info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  info.depthBiasEnable = VK_FALSE;
  return info;
}

inline VkPipelineMultisampleStateCreateInfo
pipelineMultisampleStateCreateInfo() {
  VkPipelineMultisampleStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  info.sampleShadingEnable = VK_FALSE;
  info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  return info;
}

inline VkPipelineDepthStencilStateCreateInfo
pipelineDepthStencilStateCreateInfo() {
  VkPipelineDepthStencilStateCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  info.depthTestEnable = VK_TRUE;
  info.depthWriteEnable = VK_TRUE;
  info.depthCompareOp = VK_COMPARE_OP_LESS;
  info.depthBoundsTestEnable = VK_FALSE;
  info.minDepthBounds = 0.0f;
  info.maxDepthBounds = 1.0f;
  info.stencilTestEnable = VK_FALSE;
  info.front = {};
  info.back = {};
  return info;
}

inline VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState() {
  VkPipelineColorBlendAttachmentState state = {};
  state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  state.blendEnable = VK_FALSE;
  return state;
}

inline VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
    std::span<const VkPipelineColorBlendAttachmentState> states) {
  VkPipelineColorBlendStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  info.logicOpEnable = VK_FALSE;
  info.logicOp = VK_LOGIC_OP_COPY;
  info.attachmentCount = static_cast<uint32_t>(states.size());
  info.pAttachments = states.data();
  info.blendConstants[0] = 0.0f;
  info.blendConstants[1] = 0.0f;
  info.blendConstants[2] = 0.0f;
  info.blendConstants[3] = 0.0f;
  return info;
}

inline VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
    std::span<const VkDynamicState> states) {
  VkPipelineDynamicStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  info.dynamicStateCount = static_cast<uint32_t>(states.size());
  info.pDynamicStates = states.data();
  return info;
}

inline VkBufferCreateInfo bufferCreateInfo(VkDeviceSize size,
                                           VkBufferUsageFlags usage) {
  VkBufferCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  info.size = size;
  info.usage = usage;
  info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  return info;
}

inline VkImageCreateInfo imageCreateInfo(uint32_t width, uint32_t height,
                                         VkFormat format, VkImageTiling tiling,
                                         VkImageUsageFlags usage) {
  VkImageCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  info.imageType = VK_IMAGE_TYPE_2D;
  info.extent.width = width;
  info.extent.height = height;
  info.extent.depth = 1;
  info.mipLevels = 1;
  info.arrayLayers = 1;
  info.format = format;
  info.tiling = tiling;
  info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  info.usage = usage;
  info.samples = VK_SAMPLE_COUNT_1_BIT;
  info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  return info;
}

inline VkImageMemoryBarrier imageMemoryBarrier(VkImage image,
                                               VkImageLayout oldLayout,
                                               VkImageLayout newLayout) {
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = image;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  return barrier;
}

inline VkBufferImageCopy bufferImageCopy(uint32_t width, uint32_t height) {
  VkBufferImageCopy copy{};
  copy.bufferOffset = 0;
  copy.bufferRowLength = 0;
  copy.bufferImageHeight = 0;
  copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy.imageSubresource.mipLevel = 0;
  copy.imageSubresource.baseArrayLayer = 0;
  copy.imageSubresource.layerCount = 1;
  copy.imageOffset = {0, 0, 0};
  copy.imageExtent = {width, height, 1};
  return copy;
}

inline VkImageViewCreateInfo imageViewCreateInfo(
    VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
  VkImageViewCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  info.image = image;
  info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  info.format = format;
  info.subresourceRange.aspectMask = aspectFlags;
  info.subresourceRange.baseMipLevel = 0;
  info.subresourceRange.levelCount = 1;
  info.subresourceRange.baseArrayLayer = 0;
  info.subresourceRange.layerCount = 1;
  return info;
}

inline VkSamplerCreateInfo samplerCreateInfo(
    VkPhysicalDeviceProperties const& properties) {
  VkSamplerCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  info.magFilter = VK_FILTER_LINEAR;
  info.minFilter = VK_FILTER_LINEAR;
  info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  info.anisotropyEnable = VK_FALSE;
  info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  info.unnormalizedCoordinates = VK_FALSE;
  info.compareEnable = VK_FALSE;
  info.compareOp = VK_COMPARE_OP_ALWAYS;
  info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  return info;
}
inline VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
    std::span<const VkDescriptorPoolSize> sizes, std::uint32_t maxSets) {
  VkDescriptorPoolCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  info.poolSizeCount = static_cast<uint32_t>(sizes.size());
  info.pPoolSizes = sizes.data();
  info.maxSets = maxSets;
  return info;
}

inline VkDescriptorSetAllocateInfo descriptorSetAllocateInfo(
    VkDescriptorPool pool, std::uint32_t count,
    std::span<const VkDescriptorSetLayout> layouts) {
  VkDescriptorSetAllocateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  info.descriptorPool = pool;
  info.descriptorSetCount = count;
  info.pSetLayouts = layouts.data();
  return info;
}

inline VkDescriptorBufferInfo descriptorBufferInfo(VkBuffer buffer,
                                                   VkDeviceSize offset,
                                                   VkDeviceSize range) {
  VkDescriptorBufferInfo info{};
  info.buffer = buffer;
  info.offset = offset;
  info.range = range;
  return info;
}

inline VkDescriptorImageInfo descriptorImageInfo(VkSampler sampler,
                                                 VkImageView imageView) {
  VkDescriptorImageInfo info{};
  info.sampler = sampler;
  info.imageView = imageView;
  info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  return info;
}

inline VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandPool pool,
                                                             uint32_t count) {
  VkCommandBufferAllocateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  info.commandPool = pool;
  info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  info.commandBufferCount = count;
  return info;
}

inline VkFenceCreateInfo fenceCreateInfo() {
  VkFenceCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  return info;
}

inline VkCommandBufferBeginInfo commandBufferBeginInfo(
    VkCommandBufferUsageFlags flags = 0) {
  VkCommandBufferBeginInfo info{};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  return info;
}

inline VkSubmitInfo submitInfo(
    std::span<const VkSemaphore> waitSemaphores,
    std::span<const VkPipelineStageFlags> waitDstStageMask,
    std::span<const VkCommandBuffer> commandBuffers,
    std::span<const VkSemaphore> signalSemaphores) {
  VkSubmitInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  info.waitSemaphoreCount = static_cast<std::uint32_t>(waitSemaphores.size());
  info.pWaitSemaphores = waitSemaphores.data();
  info.pWaitDstStageMask = waitDstStageMask.data();
  info.commandBufferCount = static_cast<std::uint32_t>(commandBuffers.size());
  info.pCommandBuffers = commandBuffers.data();
  info.signalSemaphoreCount =
      static_cast<std::uint32_t>(signalSemaphores.size());
  info.pSignalSemaphores = signalSemaphores.data();
  return info;
}

inline VkPresentInfoKHR presentInfoKHR(
    std::span<const VkSemaphore> waitSemaphores,
    std::span<const VkSwapchainKHR> swapchains,
    std::span<const uint32_t> indices) {
  VkPresentInfoKHR info = {};
  info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  info.waitSemaphoreCount = static_cast<std::uint32_t>(waitSemaphores.size());
  info.pWaitSemaphores = waitSemaphores.data();
  info.swapchainCount = static_cast<std::uint32_t>(swapchains.size());
  info.pSwapchains = swapchains.data();
  info.pImageIndices = indices.data();
  return info;
}

}  // namespace mpvgl::wk::initializers
