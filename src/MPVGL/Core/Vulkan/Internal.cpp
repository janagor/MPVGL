#include <array>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include <VulkanMemoryAllocator/include/vk_mem_alloc.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/trigonometric.hpp>
#include <tl/expected.hpp>
#include <vulkan/vulkan_core.h>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Camera.hpp"
#include "MPVGL/Core/UniformBufferObject.hpp"
#include "MPVGL/Core/Vulkan.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Core/Vulkan/Internal.hpp"
#include "MPVGL/Core/Vulkan/Swapchain.hpp"
#include "MPVGL/Core/Vulkan/Texture.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::vlk {

tl::expected<GLFWwindow *, Error<EngineError>> createWindow(
    char const *window_name, bool resize) {
    glfwSetErrorCallback([](int error, char const *description) {
        std::cerr << "GLFW Error (" << error << "): " << description << '\n';
    });

    if (!glfwInit()) {
        return tl::unexpected{
            Error{EngineError::VulkanInitFailed, "Failed to initialize GLFW"}};
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if (!resize) {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }

    GLFWwindow *window =
        glfwCreateWindow(800, 600, window_name, nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        return tl::unexpected{
            Error{EngineError::WindowError, "Failed to create GLFW window!"}};
    }
    return window;
}

void destroy_window_glfw(GLFWwindow *window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

VkSurfaceKHR create_surface_glfw(VkInstance instance, GLFWwindow *window,
                                 VkAllocationCallbacks *allocator) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult err =
        glfwCreateWindowSurface(instance, window, allocator, &surface);
    if (err) {
        char const *error_msg;
        int ret = glfwGetError(&error_msg);
        if (ret != 0) {
            std::cout << ret << " ";
            if (error_msg != nullptr) std::cout << error_msg;
            std::cout << "\n";
        }
        surface = VK_NULL_HANDLE;
    }
    return surface;
}

VkShaderModule createShaderModule(DeviceContext const &context,
                                  std::span<std::byte const> code) {
    std::span<u32 const> code_span{reinterpret_cast<u32 const *>(code.data()),
                                   code.size() / sizeof(u32)};

    auto create_info = initializers::shaderModuleCreateInfo(code_span);

    VkShaderModule shaderModule;
    if (context.logDevDisp.createShaderModule(&create_info, nullptr,
                                              &shaderModule) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

tl::expected<void, Error<EngineError>> createBuffer(
    Vulkan &vulkan, VkDeviceSize size, VkBufferUsageFlags usage,
    VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags allocFlags,
    VkBuffer &buffer, VmaAllocation &bufferAllocation) {
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

VkCommandBuffer beginSingleTimeCommands(Vulkan &vulkan) {
    auto &deviceContext = vulkan.deviceContext;

    auto allocInfo = initializers::commandBufferAllocateInfo(
        vulkan.data.commandPool.handle(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

    VkCommandBuffer commandBuffer;
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

    VkBufferCopy copyRegion{.size = size};
    deviceContext.logDevDisp.cmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer,
                                           1, &copyRegion);

    endSingleTimeCommands(vulkan, commandBuffer);
}

tl::expected<void, Error<EngineError>> createImage(
    Vulkan &vulkan, u32 width, u32 height, u32 mipLevels, VkFormat format,
    VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage,
    VmaAllocationCreateFlags allocFlags, VkImage &image,
    VmaAllocation &imageAllocation) {
    auto &deviceContext = vulkan.deviceContext;

    auto imageInfo = initializers::imageCreateInfo();
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
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

tl::expected<VkImageView, Error<EngineError>> createImageView(
    Vulkan &vulkan, VkImage image, VkFormat format,
    VkImageAspectFlags aspectFlags, u32 mipLevels) {
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

    VkImageView imageView;
    if (deviceContext.logDevDisp.createImageView(&viewInfo, nullptr,
                                                 &imageView) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Image View"}};
    }

    return imageView;
}

tl::expected<void, Error<EngineError>> createImageViews(Vulkan &vulkan) {
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

tl::expected<VkFormat, Error<EngineError>> findSupportedFormat(
    Vulkan &vulkan, std::vector<VkFormat> const &candidates,
    VkImageTiling tiling, VkFormatFeatureFlags features) {
    auto &deviceContext = vulkan.deviceContext;

    for (VkFormat format : candidates) {
        VkFormatProperties props;
        deviceContext.instDisp.getPhysicalDeviceFormatProperties(
            deviceContext.logicalDevice.physical_device, format, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR &&
            (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                   (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                "Failed to find supported Format"}};
}

tl::expected<VkFormat, Error<EngineError>> findDepthFormat(Vulkan &vulkan) {
    return findSupportedFormat(
        vulkan,
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
         VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool has_stencil_component(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
           format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void cleanupSwapChain(Vulkan &vulkan) {
    auto &deviceContext = vulkan.deviceContext;
    auto &swapchainContext = vulkan.swapchainContext;

    swapchainContext.depthTexture = Texture{};
    for (auto framebuffer : swapchainContext.framebuffers) {
        deviceContext.logDevDisp.destroyFramebuffer(framebuffer, nullptr);
    }
    swapchainContext.framebuffers.clear();
    swapchainContext.swapchain = Swapchain();
}

tl::expected<void, Error<EngineError>> recreateSwapchain(Vulkan &vulkan) {
    auto &deviceContext = vulkan.deviceContext;
    auto &swapchainContext = vulkan.swapchainContext;

    deviceContext.logDevDisp.deviceWaitIdle();

    swapchainContext.depthTexture = Texture{};
    for (auto framebuffer : swapchainContext.framebuffers) {
        deviceContext.logDevDisp.destroyFramebuffer(framebuffer, nullptr);
    }
    swapchainContext.framebuffers.clear();

    return swapchainContext.swapchain.recreate(vulkan.deviceContext)
        .and_then([&]() { return createDepthResources(vulkan); })
        .and_then([&]() { return createFramebuffers(vulkan); })
        .and_then([&]() -> tl::expected<void, Error<EngineError>> {
            auto imageCount = swapchainContext.swapchain.imageCount();
            vulkan.data.imageInFlight.assign(imageCount, VK_NULL_HANDLE);

            vulkan.data.finishedSemaphores.clear();
            vulkan.data.finishedSemaphores.reserve(imageCount);

            for (size_t i = 0; i < imageCount; ++i) {
                if (auto semRes = Semaphore::create(deviceContext);
                    semRes.has_value()) {
                    vulkan.data.finishedSemaphores.push_back(
                        std::move(semRes.value()));
                } else {
                    return tl::unexpected{semRes.error()};
                }
            }
            return {};
        });
}

tl::expected<void, Error<EngineError>> recordCommandBuffer(
    Vulkan &vulkan, VkCommandBuffer command_buffer, u32 image_index) {
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
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

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
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (f32)swapchainContext.swapchain.extent().width;
    viewport.height = (f32)swapchainContext.swapchain.extent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    deviceContext.logDevDisp.cmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapchainContext.swapchain.extent();
    deviceContext.logDevDisp.cmdSetScissor(command_buffer, 0, 1, &scissor);

    for (auto const &object : sceneContext.renderables) {
        deviceContext.logDevDisp.cmdBindDescriptorSets(
            command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineContext.graphicsPipeline.layout(), 0, 1,
            &object.material->descriptorSets[vulkan.data.currentFrame], 0,
            nullptr);

        VkBuffer vertexBuffers[] = {object.model->vertexBuffer().handle()};
        VkDeviceSize offsets[] = {0};
        deviceContext.logDevDisp.cmdBindVertexBuffers(command_buffer, 0, 1,
                                                      vertexBuffers, offsets);

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

    static auto start_time = std::chrono::high_resolution_clock::now();

    auto current_time = std::chrono::high_resolution_clock::now();
    f32 time = std::chrono::duration<f32, std::chrono::seconds::period>(
                   current_time - start_time)
                   .count();

    UniformBufferObject ubo{};
    ubo.view = sceneContext.camera.getViewMatrix();
    ubo.projection = glm::perspective(
        glm::radians(sceneContext.camera.Zoom),
        swapchainContext.swapchain.extent().width /
            static_cast<f32>(swapchainContext.swapchain.extent().height),
        0.1f, 10.0f);
    ubo.projection[1][1] *= -1;

    memcpy(vulkan.data.frames.at(current_image).uniformBufferMapped, &ubo,
           sizeof(ubo));
}

}  // namespace mpvgl::vlk
