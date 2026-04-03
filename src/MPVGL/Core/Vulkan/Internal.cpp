#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <VulkanMemoryAllocator/include/vk_mem_alloc.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/trigonometric.hpp>
#include <tl/expected.hpp>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Camera.hpp"
#include "MPVGL/Core/Renderer.hpp"
#include "MPVGL/Core/UniformBufferObject.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Core/Vulkan/Internal.hpp"
#include "MPVGL/Core/Vulkan/Swapchain.hpp"
#include "MPVGL/Core/Vulkan/SyncObjects.hpp"
#include "MPVGL/Core/Vulkan/Texture.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"
#include "MPVGL/Graphics/Extent.hpp"
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl::vlk {

auto createShaderModule(DeviceContext const &context,
                        std::span<std::byte const> code) -> VkShaderModule {
    std::span<u32 const> const code_span{
        reinterpret_cast<u32 const *>(code.data()), code.size() / sizeof(u32)};

    auto create_info = initializers::shaderModuleCreateInfo(code_span);

    VkShaderModule shaderModule = nullptr;
    if (context.logDevDisp.createShaderModule(&create_info, nullptr,
                                              &shaderModule) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

auto createBuffer(Vulkan &vulkan, VkDeviceSize size, VkBufferUsageFlags usage,
                  VmaMemoryUsage memoryUsage,
                  VmaAllocationCreateFlags allocFlags, VkBuffer &buffer,
                  VmaAllocation &bufferAllocation)
    -> tl::expected<void, Error<EngineError>> {
    auto bufferInfo = initializers::bufferCreateInfo(size, usage);

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = allocFlags;

    if (vmaCreateBuffer(vulkan.deviceContext.allocator, &bufferInfo, &allocInfo,
                        &buffer, &bufferAllocation, nullptr) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Buffer via VMA"}};
    }
    return {};
}

auto beginSingleTimeCommands(Vulkan &vulkan) -> VkCommandBuffer {
    auto &deviceContext = vulkan.deviceContext;

    auto allocInfo = initializers::commandBufferAllocateInfo(
        vulkan.data.commandPool.handle(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

    VkCommandBuffer commandBuffer = nullptr;
    deviceContext.logDevDisp.allocateCommandBuffers(&allocInfo, &commandBuffer);

    auto beginInfo = initializers::commandBufferBeginInfo(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    deviceContext.logDevDisp.beginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(Vulkan &vulkan, VkCommandBuffer commandBuffer) {
    auto &deviceContext = vulkan.deviceContext;

    deviceContext.logDevDisp.endCommandBuffer(commandBuffer);

    auto submitInfo = initializers::submitInfo({}, {}, {&commandBuffer, 1}, {});
    deviceContext.logDevDisp.queueSubmit(deviceContext.graphicsQueue, 1,
                                         &submitInfo, VK_NULL_HANDLE);
    deviceContext.logDevDisp.queueWaitIdle(deviceContext.graphicsQueue);
    deviceContext.logDevDisp.freeCommandBuffers(
        vulkan.data.commandPool.handle(), 1, &commandBuffer);
}

void copy_buffer(Vulkan &vulkan, VkBuffer srcBuffer, VkBuffer dstBuffer,
                 VkDeviceSize size) {
    auto &deviceContext = vulkan.deviceContext;

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(vulkan);

    VkBufferCopy const copyRegion{.size = size};
    deviceContext.logDevDisp.cmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer,
                                           1, &copyRegion);

    endSingleTimeCommands(vulkan, commandBuffer);
}

auto createImage(Vulkan &vulkan, Extent2D const &extent, u32 mipLevels,
                 VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                 VmaMemoryUsage memoryUsage,
                 VmaAllocationCreateFlags allocFlags, VkImage &image,
                 VmaAllocation &imageAllocation)
    -> tl::expected<void, Error<EngineError>> {
    auto &deviceContext = vulkan.deviceContext;

    auto imageInfo = initializers::imageCreateInfo();
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = extent.width;
    imageInfo.extent.height = extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = allocFlags;

    if (vmaCreateImage(deviceContext.allocator, &imageInfo, &allocInfo, &image,
                       &imageAllocation, nullptr) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Image via VMA"}};
    }
    return {};
}

auto createImageView(Vulkan &vulkan, VkImage image, VkFormat format,
                     VkImageAspectFlags aspectFlags, u32 mipLevels)
    -> tl::expected<VkImageView, Error<EngineError>> {
    auto &deviceContext = vulkan.deviceContext;

    auto subresourceRange = VkImageSubresourceRange{
        .aspectMask = aspectFlags,
        .baseMipLevel = 0,
        .levelCount = mipLevels,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    auto viewInfo = initializers::imageViewCreateInfo(
        image, VK_IMAGE_VIEW_TYPE_2D, format, subresourceRange);

    VkImageView imageView = nullptr;
    if (deviceContext.logDevDisp.createImageView(&viewInfo, nullptr,
                                                 &imageView) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Image View"}};
    }

    return imageView;
}

auto createImageViews(Vulkan &vulkan)
    -> tl::expected<void, Error<EngineError>> {
    auto &swapchainContext = vulkan.swapchainContext;

    swapchainContext.swapchain.imageViews().resize(
        swapchainContext.swapchain.images().size());
    for (u32 i = 0; i < swapchainContext.swapchain.images().size(); ++i) {
        auto imageView = createImageView(
            vulkan, swapchainContext.swapchain.images().at(i),
            swapchainContext.swapchain.format(), VK_IMAGE_ASPECT_COLOR_BIT, 1);
        if (!imageView.has_value()) {
            return tl::unexpected{imageView.error()};
        }
        swapchainContext.swapchain.imageViews().at(i) = imageView.value();
    }
    return {};
}

auto findSupportedFormat(Vulkan &vulkan,
                         std::vector<VkFormat> const &candidates,
                         VkImageTiling tiling, VkFormatFeatureFlags features)
    -> tl::expected<VkFormat, Error<EngineError>> {
    auto &deviceContext = vulkan.deviceContext;

    for (VkFormat format : candidates) {
        VkFormatProperties props;
        deviceContext.instDisp.getPhysicalDeviceFormatProperties(
            deviceContext.logicalDevice.physical_device, format, &props);

        bool const isLinearOk =
            (tiling == VK_IMAGE_TILING_LINEAR &&
             (props.linearTilingFeatures & features) == features);
        bool const isOptimalOk =
            (tiling == VK_IMAGE_TILING_OPTIMAL &&
             (props.optimalTilingFeatures & features) == features);

        if (isLinearOk || isOptimalOk) {
            return format;
        }
    }
    return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                "Failed to find supported Format"}};
}

auto findDepthFormat(Vulkan &vulkan)
    -> tl::expected<VkFormat, Error<EngineError>> {
    return findSupportedFormat(
        vulkan,
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
         VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

auto has_stencil_component(VkFormat format) -> bool {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
           format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void cleanupSwapChain(Vulkan &vulkan) {
    auto &deviceContext = vulkan.deviceContext;
    auto &swapchainContext = vulkan.swapchainContext;

    swapchainContext.depthTexture = Texture{};
    for (auto *framebuffer : swapchainContext.framebuffers) {
        deviceContext.logDevDisp.destroyFramebuffer(framebuffer, nullptr);
    }
    swapchainContext.framebuffers.clear();
    swapchainContext.swapchain = Swapchain();
}

auto recreateSwapchain(Renderer &renderer)
    -> tl::expected<void, Error<EngineError>> {
    auto &deviceContext = renderer.vulkan().deviceContext;
    auto &swapchainContext = renderer.vulkan().swapchainContext;

    deviceContext.logDevDisp.deviceWaitIdle();

    swapchainContext.depthTexture = Texture{};
    for (auto *framebuffer : swapchainContext.framebuffers) {
        deviceContext.logDevDisp.destroyFramebuffer(framebuffer, nullptr);
    }
    swapchainContext.framebuffers.clear();

    return swapchainContext.swapchain.recreate(renderer.vulkan().deviceContext)
        .and_then([&]() -> tl::expected<void, Error<EngineError>> {
            return renderer.createDepthResources();
        })
        .and_then([&]() -> tl::expected<void, Error<EngineError>> {
            return renderer.createFramebuffers();
        })
        .and_then([&]() -> tl::expected<void, Error<EngineError>> {
            auto imageCount = swapchainContext.swapchain.imageCount();
            renderer.vulkan().data.imageInFlight.assign(imageCount,
                                                        VK_NULL_HANDLE);

            renderer.vulkan().data.finishedSemaphores.clear();
            renderer.vulkan().data.finishedSemaphores.reserve(imageCount);

            for (size_t i = 0; i < imageCount; ++i) {
                if (auto semRes = Semaphore::create(deviceContext);
                    semRes.has_value()) {
                    renderer.vulkan().data.finishedSemaphores.push_back(
                        std::move(semRes.value()));
                } else {
                    return tl::unexpected{semRes.error()};
                }
            }
            return {};
        });
}

auto recordCommandBuffer(Vulkan &vulkan, VkCommandBuffer command_buffer,
                         u32 image_index)
    -> tl::expected<void, Error<EngineError>> {
    auto &sceneContext = vulkan.sceneContext;
    auto &deviceContext = vulkan.deviceContext;
    auto &pipelineContext = vulkan.pipelineContext;
    auto &swapchainContext = vulkan.swapchainContext;

    auto beginInfo = initializers::commandBufferBeginInfo();
    if (deviceContext.logDevDisp.beginCommandBuffer(command_buffer,
                                                    &beginInfo) != VK_SUCCESS) {
        return tl::unexpected{
            Error{EngineError::VulkanRuntimeError,
                  "Failed to begin recording Command Buffer"}};
    }

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0F, 0.0F, 0.0F, 1.0F}};
    clearValues[1].depthStencil = {.depth = 1.0F, .stencil = 0};

    auto renderPassInfo = initializers::renderPassBeginInfo(
        swapchainContext.renderPass.handle(),
        swapchainContext.framebuffers.at(image_index),
        VkRect2D{VkOffset2D{0, 0}, swapchainContext.swapchain.extent()},
        clearValues);

    deviceContext.logDevDisp.cmdBeginRenderPass(command_buffer, &renderPassInfo,
                                                VK_SUBPASS_CONTENTS_INLINE);

    deviceContext.logDevDisp.cmdBindPipeline(
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineContext.graphicsPipeline.handle());

    VkViewport viewport = {};
    viewport.x = 0.0F;
    viewport.y = 0.0F;
    viewport.width =
        static_cast<f32>(swapchainContext.swapchain.extent().width);
    viewport.height =
        static_cast<f32>(swapchainContext.swapchain.extent().height);
    viewport.minDepth = 0.0F;
    viewport.maxDepth = 1.0F;
    deviceContext.logDevDisp.cmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {.x = 0, .y = 0};
    scissor.extent = swapchainContext.swapchain.extent();
    deviceContext.logDevDisp.cmdSetScissor(command_buffer, 0, 1, &scissor);

    for (auto const &object : sceneContext.renderables) {
        deviceContext.logDevDisp.cmdBindDescriptorSets(
            command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineContext.graphicsPipeline.layout(), 0, 1,
            &object.material->descriptorSets[vulkan.data.currentFrame], 0,
            nullptr);

        constexpr VkDeviceSize DEFAULT_OFFSET = 0;
        auto const vertexBuffers =
            std::array{object.model->vertexBuffer().handle()};
        auto const offsets = std::array{DEFAULT_OFFSET};
        deviceContext.logDevDisp.cmdBindVertexBuffers(
            command_buffer, 0, 1, vertexBuffers.data(), offsets.data());

        deviceContext.logDevDisp.cmdBindIndexBuffer(
            command_buffer, object.model->indexBuffer().handle(), 0,
            VK_INDEX_TYPE_UINT32);

        glm::mat4 modelMatrix = object.transformMatrix;
        deviceContext.logDevDisp.cmdPushConstants(
            command_buffer, pipelineContext.graphicsPipeline.layout(),
            VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &modelMatrix);

        deviceContext.logDevDisp.cmdDrawIndexed(
            command_buffer, object.model->indexCount(), 1, 0, 0, 0);
    }

    deviceContext.logDevDisp.cmdEndRenderPass(command_buffer);

    if (deviceContext.logDevDisp.endCommandBuffer(command_buffer) !=
        VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to record Command Buffer"}};
    }

    return {};
}

void updateUniformBuffer(Vulkan &vulkan, u32 current_image) {
    auto &sceneContext = vulkan.sceneContext;
    auto &swapchainContext = vulkan.swapchainContext;

    constexpr f32 NEAR_PLANE = 0.1F;
    constexpr f32 FAR_PLANE = 10.0F;
    constexpr f32 VULKAN_Y_FLIP = -1.0F;

    UniformBufferObject ubo{};
    ubo.view = sceneContext.camera.getViewMatrix();
    ubo.projection = glm::perspective(
        glm::radians(static_cast<f32>(sceneContext.camera.zoom())),
        static_cast<f32>(swapchainContext.swapchain.extent().width) /
            static_cast<f32>(swapchainContext.swapchain.extent().height),
        NEAR_PLANE, FAR_PLANE);

    ubo.projection[1][1] *= VULKAN_Y_FLIP;

    memcpy(vulkan.data.frames.at(current_image).uniformBufferMapped, &ubo,
           sizeof(ubo));
}

}  // namespace mpvgl::vlk
