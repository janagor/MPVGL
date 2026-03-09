#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include <VulkanMemoryAllocator/include/vk_mem_alloc.h>
#include <glm/glm.hpp>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Camera.hpp"
#include "MPVGL/Core/Error.hpp"
#include "MPVGL/Core/UniformBufferObject.hpp"
#include "MPVGL/Core/Vulkan.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Core/Vulkan/Internal.hpp"

#include "tl/expected.hpp"

namespace mpvgl::vlk {

tl::expected<GLFWwindow *, Error> createWindow(const char *window_name,
                                               bool resize) {
    glfwSetErrorCallback([](int error, const char *description) {
        fprintf(stderr, "GLFW Error (%d): %s\n", error, description);
    });

    if (!glfwInit()) {
        return tl::unexpected<Error>{EngineError::VulkanInitFailed,
                                     "Failed to initialize GLFW"};
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if (!resize) {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }

    GLFWwindow *window =
        glfwCreateWindow(800, 600, window_name, nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        return tl::unexpected<Error>{EngineError::WindowError,
                                     "Failed to create GLFW window!"};
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
        const char *error_msg;
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

tl::expected<std::vector<char>, Error> readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        return tl::unexpected<Error>{EngineError::VulkanRuntimeError,
                                     "Failed to open a File"};
    }

    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(file_size));

    file.close();

    return buffer;
}

// TODO: createShaderModule(Vulkan&vulkan, std::span<const std::uint32_t>
// &code);
VkShaderModule createShaderModule(Vulkan &vulkan,
                                  const std::vector<char> &code) {
    std::span<const std::uint32_t> code_span{
        reinterpret_cast<const std::uint32_t *>(code.data()),
        code.size() / sizeof(std::uint32_t)};
    auto create_info = initializers::shaderModuleCreateInfo(code_span);

    VkShaderModule shaderModule;
    if (vulkan.deviceContext.logDevDisp.createShaderModule(
            &create_info, nullptr, &shaderModule) != VK_SUCCESS) {
        return VK_NULL_HANDLE;  // failed to create shader module
    }

    return shaderModule;
}

tl::expected<void, Error> createBuffer(Vulkan &vulkan, VkDeviceSize size,
                                       VkBufferUsageFlags usage,
                                       VmaMemoryUsage memoryUsage,
                                       VmaAllocationCreateFlags allocFlags,
                                       VkBuffer &buffer,
                                       VmaAllocation &bufferAllocation) {
    auto bufferInfo = initializers::bufferCreateInfo(size, usage);

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = allocFlags;

    if (vmaCreateBuffer(vulkan.deviceContext.allocator, &bufferInfo, &allocInfo,
                        &buffer, &bufferAllocation, nullptr) != VK_SUCCESS) {
        return tl::unexpected<Error>{EngineError::VulkanRuntimeError,
                                     "Failed to create Buffer via VMA"};
    }
    return {};
}

VkCommandBuffer beginSingleTimeCommands(Vulkan &vulkan) {
    auto allocInfo = initializers::commandBufferAllocateInfo(
        vulkan.data.command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

    VkCommandBuffer commandBuffer;
    vulkan.deviceContext.logDevDisp.allocateCommandBuffers(&allocInfo,
                                                           &commandBuffer);

    auto beginInfo = initializers::commandBufferBeginInfo(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    vulkan.deviceContext.logDevDisp.beginCommandBuffer(commandBuffer,
                                                       &beginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(Vulkan &vulkan, VkCommandBuffer commandBuffer) {
    vulkan.deviceContext.logDevDisp.endCommandBuffer(commandBuffer);

    auto submitInfo = initializers::submitInfo({}, {}, {&commandBuffer, 1}, {});
    vulkan.deviceContext.logDevDisp.queueSubmit(
        vulkan.deviceContext.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vulkan.deviceContext.logDevDisp.queueWaitIdle(
        vulkan.deviceContext.graphicsQueue);
    vulkan.deviceContext.logDevDisp.freeCommandBuffers(vulkan.data.command_pool,
                                                       1, &commandBuffer);
}

void copy_buffer(Vulkan &vulkan, VkBuffer srcBuffer, VkBuffer dstBuffer,
                 VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(vulkan);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vulkan.deviceContext.logDevDisp.cmdCopyBuffer(commandBuffer, srcBuffer,
                                                  dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(vulkan, commandBuffer);
}

tl::expected<void, Error> createImage(
    Vulkan &vulkan, uint32_t width, uint32_t height, uint32_t mipLevels,
    VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
    VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags allocFlags,
    VkImage &image, VmaAllocation &imageAllocation) {
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

    if (vmaCreateImage(vulkan.deviceContext.allocator, &imageInfo, &allocInfo,
                       &image, &imageAllocation, nullptr) != VK_SUCCESS) {
        return tl::unexpected<Error>{EngineError::VulkanRuntimeError,
                                     "Failed to create Image via VMA"};
    }
    return {};
}

tl::expected<VkImageView, Error> createImageView(Vulkan &vulkan, VkImage image,
                                                 VkFormat format,
                                                 VkImageAspectFlags aspectFlags,
                                                 uint32_t mipLevels) {
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
    if (vulkan.deviceContext.logDevDisp.createImageView(
            &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        return tl::unexpected<Error>{EngineError::VulkanRuntimeError,
                                     "Failed to create Image View"};
    }

    return imageView;
}

tl::expected<void, Error> createImageViews(Vulkan &vulkan) {
    vulkan.swapchainContext.swapchain.imageViews().resize(
        vulkan.swapchainContext.swapchain.images().size());
    for (uint32_t i = 0; i < vulkan.swapchainContext.swapchain.images().size();
         ++i) {
        auto imageView = createImageView(
            vulkan, vulkan.swapchainContext.swapchain.images().at(i),
            vulkan.swapchainContext.swapchain.format(),
            VK_IMAGE_ASPECT_COLOR_BIT, 1);
        if (!imageView.has_value()) {
            return tl::unexpected<Error>(imageView.error());
        }
        vulkan.swapchainContext.swapchain.imageViews().at(i) =
            imageView.value();
    }
    return {};
}

tl::expected<VkFormat, Error> findSupportedFormat(
    Vulkan &vulkan, const std::vector<VkFormat> &candidates,
    VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vulkan.deviceContext.instDisp.getPhysicalDeviceFormatProperties(
            vulkan.deviceContext.logicalDevice.physical_device, format, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR &&
            (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                   (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    return tl::unexpected<Error>{EngineError::VulkanRuntimeError,
                                 "Failed to find supported Format"};
}

tl::expected<VkFormat, Error> findDepthFormat(Vulkan &vulkan) {
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
    vulkan.swapchainContext.depthTexture = Texture{};
    for (auto framebuffer : vulkan.swapchainContext.framebuffers) {
        vulkan.deviceContext.logDevDisp.destroyFramebuffer(framebuffer,
                                                           nullptr);
    }
    vulkan.swapchainContext.framebuffers.clear();
    vulkan.swapchainContext.swapchain = Swapchain();
}

tl::expected<void, Error> recreateSwapchain(Vulkan &vulkan) {
    vulkan.deviceContext.logDevDisp.deviceWaitIdle();

    vulkan.swapchainContext.depthTexture = Texture{};
    for (auto framebuffer : vulkan.swapchainContext.framebuffers) {
        vulkan.deviceContext.logDevDisp.destroyFramebuffer(framebuffer,
                                                           nullptr);
    }
    vulkan.swapchainContext.framebuffers.clear();

    return vulkan.swapchainContext.swapchain.recreate(vulkan.deviceContext)
        .and_then([&vulkan]() { return createDepthResources(vulkan); })
        .and_then([&vulkan]() { return createFramebuffers(vulkan); })
        .and_then([&vulkan]() -> tl::expected<void, Error> {
            auto imageCount = vulkan.swapchainContext.swapchain.imageCount();
            vulkan.data.image_in_flight.assign(imageCount, VK_NULL_HANDLE);
            for (auto &sem : vulkan.data.finishedSemaphores) {
                vulkan.deviceContext.logDevDisp.destroySemaphore(sem, nullptr);
            }
            vulkan.data.finishedSemaphores.clear();
            vulkan.data.finishedSemaphores.resize(imageCount, VK_NULL_HANDLE);

            auto semaphoreInfo = initializers::semaphoreCreateInfo();
            for (size_t i = 0; i < imageCount; ++i) {
                if (vulkan.deviceContext.logDevDisp.createSemaphore(
                        &semaphoreInfo, nullptr,
                        &vulkan.data.finishedSemaphores[i]) != VK_SUCCESS) {
                    return tl::unexpected(
                        Error(EngineError::VulkanRuntimeError,
                              "Failed to recreate semaphores"));
                }
            }
            return {};
        });
}

tl::expected<void, Error> recordCommandBuffer(Vulkan &vulkan,
                                              VkCommandBuffer command_buffer,
                                              uint32_t image_index) {
    auto beginInfo = initializers::commandBufferBeginInfo();
    if (vulkan.deviceContext.logDevDisp.beginCommandBuffer(
            command_buffer, &beginInfo) != VK_SUCCESS) {
        return tl::unexpected<Error>{
            EngineError::VulkanRuntimeError,
            "Failed to begin recording Command Buffer"};
    }

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    auto renderPassInfo = initializers::renderPassBeginInfo(
        vulkan.swapchainContext.renderPass,
        vulkan.swapchainContext.framebuffers.at(image_index),
        VkRect2D{VkOffset2D{0, 0}, vulkan.swapchainContext.swapchain.extent()},
        clearValues);

    vulkan.deviceContext.logDevDisp.cmdBeginRenderPass(
        command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vulkan.deviceContext.logDevDisp.cmdBindPipeline(
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        vulkan.pipelineContext.graphicsPipeline);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)vulkan.swapchainContext.swapchain.extent().width;
    viewport.height = (float)vulkan.swapchainContext.swapchain.extent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vulkan.deviceContext.logDevDisp.cmdSetViewport(command_buffer, 0, 1,
                                                   &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = vulkan.swapchainContext.swapchain.extent();
    vulkan.deviceContext.logDevDisp.cmdSetScissor(command_buffer, 0, 1,
                                                  &scissor);

    VkBuffer vertexBuffers[] = {vulkan.sceneContext.vertexBuffer.handle()};
    VkDeviceSize offsets[] = {0};
    vulkan.deviceContext.logDevDisp.cmdBindVertexBuffers(
        command_buffer, 0, 1, vertexBuffers, offsets);

    vulkan.deviceContext.logDevDisp.cmdBindIndexBuffer(
        command_buffer, vulkan.sceneContext.indexBuffer.handle(), 0,
        VK_INDEX_TYPE_UINT32);

    vulkan.deviceContext.logDevDisp.cmdBindDescriptorSets(
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        vulkan.pipelineContext.pipelineLayout, 0, 1,
        &vulkan.data.frames.at(vulkan.data.current_frame).descriptorSet, 0,
        nullptr);

    vulkan.deviceContext.logDevDisp.cmdDrawIndexed(
        command_buffer,
        static_cast<uint32_t>(vulkan.sceneContext.indices.size()), 1, 0, 0, 0);

    vulkan.deviceContext.logDevDisp.cmdEndRenderPass(command_buffer);

    if (vulkan.deviceContext.logDevDisp.endCommandBuffer(command_buffer) !=
        VK_SUCCESS) {
        return tl::unexpected<Error>{EngineError::VulkanRuntimeError,
                                     "Failed to record Command Buffer"};
    }
    return {};
}

void updateUniformBuffer(Vulkan &vulkan, uint32_t current_image) {
    static auto start_time = std::chrono::high_resolution_clock::now();

    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     current_time - start_time)
                     .count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
                            glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = vulkan.sceneContext.camera.getViewMatrix();
    ubo.projection = glm::perspective(
        glm::radians(vulkan.sceneContext.camera.Zoom),
        vulkan.swapchainContext.swapchain.extent().width /
            static_cast<float>(
                vulkan.swapchainContext.swapchain.extent().height),
        0.1f, 10.0f);
    ubo.projection[1][1] *= -1;

    memcpy(vulkan.data.frames.at(current_image).uniformBufferMapped, &ubo,
           sizeof(ubo));
}

}  // namespace mpvgl::vlk
