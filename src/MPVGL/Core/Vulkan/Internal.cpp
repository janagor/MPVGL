#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

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

GLFWwindow *create_window_glfw(const char *window_name, bool resize) {
    glfwSetErrorCallback([](int error, const char *description) {
        fprintf(stderr, "GLFW Error (%d): %s\n", error, description);
    });
    if (glfwInit()) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // if (!resize) glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        return glfwCreateWindow(800, 600, window_name, NULL, NULL);
    }
    throw std::runtime_error("Nie udało się zainicjować GLFW!");
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

std::vector<char> readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
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

uint32_t find_memory_type(Vulkan &vulkan, uint32_t typeFilter,
                          VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vulkan.deviceContext.instDisp.getPhysicalDeviceMemoryProperties(
        vulkan.deviceContext.logicalDevice.physical_device, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) ==
                properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

tl::expected<void, Error> createBuffer(Vulkan &vulkan, VkDeviceSize size,
                                       VkBufferUsageFlags usage,
                                       VkMemoryPropertyFlags properties,
                                       VkBuffer &buffer,
                                       VkDeviceMemory &bufferMemory) {
    auto bufferInfo = initializers::bufferCreateInfo(size, usage);
    if (vulkan.deviceContext.logDevDisp.createBuffer(&bufferInfo, nullptr,
                                                     &buffer) != VK_SUCCESS) {
        return tl::unexpected<Error>{EngineError::VulkanRuntimeError,
                                     "Failed to create Buffer"};
    }

    VkMemoryRequirements memRequirements;
    vulkan.deviceContext.logDevDisp.getBufferMemoryRequirements(
        buffer, &memRequirements);

    auto allocInfo = initializers::memoryAllocateInfo(
        memRequirements.size,
        find_memory_type(vulkan, memRequirements.memoryTypeBits, properties));

    if (vulkan.deviceContext.logDevDisp.allocateMemory(
            &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        return tl::unexpected<Error>{EngineError::VulkanRuntimeError,
                                     "Failed to allocate Buffer Memory"};
    }

    vulkan.deviceContext.logDevDisp.bindBufferMemory(buffer, bufferMemory, 0);
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

tl::expected<void, Error> createImage(Vulkan &vulkan, uint32_t width,
                                      uint32_t height, uint32_t mipLevels,
                                      VkFormat format, VkImageTiling tiling,
                                      VkImageUsageFlags usage,
                                      VkMemoryPropertyFlags properties,
                                      VkImage &image,
                                      VkDeviceMemory &imageMemory) {
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

    if (vulkan.deviceContext.logDevDisp.createImage(&imageInfo, nullptr,
                                                    &image) != VK_SUCCESS) {
        return tl::unexpected<Error>{EngineError::VulkanRuntimeError,
                                     "Failed to create Image"};
    }

    VkMemoryRequirements memRequirements;
    vulkan.deviceContext.logDevDisp.getImageMemoryRequirements(
        image, &memRequirements);

    auto allocInfo = initializers::memoryAllocateInfo(
        memRequirements.size,
        find_memory_type(vulkan, memRequirements.memoryTypeBits, properties));

    if (vulkan.deviceContext.logDevDisp.allocateMemory(
            &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        return tl::unexpected<Error>{EngineError::VulkanRuntimeError,
                                     "Failed to allocate Image Memory"};
    }
    vulkan.deviceContext.logDevDisp.bindImageMemory(image, imageMemory, 0);
    return {};
}

void transition_image_layout(Vulkan &vulkan, VkImage image, VkFormat format,
                             VkImageLayout oldLayout, VkImageLayout newLayout,
                             uint32_t mipLevels) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(vulkan);

    auto subresourceRange = VkImageSubresourceRange{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = mipLevels,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    auto barrier = initializers::imageMemoryBarrier(oldLayout, newLayout, image,
                                                    subresourceRange);

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }
    vulkan.deviceContext.logDevDisp.cmdPipelineBarrier(
        commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr,
        1, &barrier);

    endSingleTimeCommands(vulkan, commandBuffer);
}
void generateMipmaps(Vulkan &vulkan, VkImage image, VkFormat imageFormat,
                     int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
    VkFormatProperties formatProperties;
    vulkan.deviceContext.instDisp.getPhysicalDeviceFormatProperties(
        vulkan.deviceContext.logicalDevice.physical_device, imageFormat,
        &formatProperties);
    if (!(formatProperties.optimalTilingFeatures &
          VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error(
            "texture image format does not support linear blitting!");
    }
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(vulkan);
    auto subresourceRange = VkImageSubresourceRange{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    auto barrier = initializers::imageMemoryBarrier(
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, subresourceRange);

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vulkan.deviceContext.logDevDisp.cmdPipelineBarrier(
            commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
            &barrier);
        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1,
                              mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;
        vulkan.deviceContext.logDevDisp.cmdBlitImage(
            commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vulkan.deviceContext.logDevDisp.cmdPipelineBarrier(
            commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
            &barrier);
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vulkan.deviceContext.logDevDisp.cmdPipelineBarrier(
        commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
        &barrier);

    endSingleTimeCommands(vulkan, commandBuffer);
}

void copy_buffer_to_image(Vulkan &vulkan, VkBuffer buffer, VkImage image,
                          uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(vulkan);
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};
    vulkan.deviceContext.logDevDisp.cmdCopyBufferToImage(
        commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
        &region);

    endSingleTimeCommands(vulkan, commandBuffer);
}

VkImageView createImageView(Vulkan &vulkan, VkImage image, VkFormat format,
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
        throw std::runtime_error("failed to create image view!");
    }

    return imageView;
}

int create_image_views(Vulkan &vulkan) {
    vulkan.swapchainContext.swapchainImageViews.resize(
        vulkan.swapchainContext.swapchainImages.size());
    for (uint32_t i = 0; i < vulkan.swapchainContext.swapchainImages.size();
         ++i) {
        vulkan.swapchainContext.swapchainImageViews.at(i) = createImageView(
            vulkan, vulkan.swapchainContext.swapchainImages.at(i),
            vulkan.swapchainContext.swapchain.image_format,
            VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
    return 0;
}

VkFormat find_supported_format(Vulkan &vulkan,
                               const std::vector<VkFormat> &candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features) {
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
    throw std::runtime_error("failed to find supported format!");
}

VkFormat find_depth_format(Vulkan &vulkan) {
    return find_supported_format(
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
    if (vulkan.swapchainContext.depthImageView != VK_NULL_HANDLE) {
        vulkan.deviceContext.logDevDisp.destroyImageView(
            vulkan.swapchainContext.depthImageView, nullptr);
        vulkan.deviceContext.logDevDisp.destroyImage(
            vulkan.swapchainContext.depthImage, nullptr);
        vulkan.deviceContext.logDevDisp.freeMemory(
            vulkan.swapchainContext.depthImageMemory, nullptr);
    }

    for (auto framebuffer : vulkan.swapchainContext.framebuffers) {
        vulkan.deviceContext.logDevDisp.destroyFramebuffer(framebuffer,
                                                           nullptr);
    }

    vulkan.swapchainContext.swapchain.destroy_image_views(
        vulkan.swapchainContext.swapchainImageViews);

    vkb::destroy_swapchain(vulkan.swapchainContext.swapchain);
}

int recreate_swapchain(Vulkan &vulkan) {
    vulkan.deviceContext.logDevDisp.deviceWaitIdle();

    if (vulkan.swapchainContext.depthImageView != VK_NULL_HANDLE) {
        vulkan.deviceContext.logDevDisp.destroyImageView(
            vulkan.swapchainContext.depthImageView, nullptr);
        vulkan.deviceContext.logDevDisp.destroyImage(
            vulkan.swapchainContext.depthImage, nullptr);
        vulkan.deviceContext.logDevDisp.freeMemory(
            vulkan.swapchainContext.depthImageMemory, nullptr);
    }

    vulkan.deviceContext.logDevDisp.destroyCommandPool(vulkan.data.command_pool,
                                                       nullptr);
    for (auto framebuffer : vulkan.swapchainContext.framebuffers) {
        vulkan.deviceContext.logDevDisp.destroyFramebuffer(framebuffer,
                                                           nullptr);
    }

    vulkan.swapchainContext.swapchain.destroy_image_views(
        vulkan.swapchainContext.swapchainImageViews);
    vulkan.swapchainContext.framebuffers.clear();
    vulkan.swapchainContext.swapchainImageViews.clear();

    if (!create_swapchain(vulkan).has_value()) return -1;
    vulkan.swapchainContext.swapchainImages =
        vulkan.swapchainContext.swapchain.get_images().value();
    if (!createDepthResources(vulkan).has_value()) return -1;
    if (0 != create_image_views(vulkan)) return -1;
    if (!createFramebuffers(vulkan).has_value()) return -1;
    if (!createCommandPool(vulkan).has_value()) return -1;
    if (!createCommandBuffers(vulkan).has_value()) return -1;

    return 0;
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
        VkRect2D{VkOffset2D{0, 0}, vulkan.swapchainContext.swapchain.extent},
        clearValues);

    vulkan.deviceContext.logDevDisp.cmdBeginRenderPass(
        command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vulkan.deviceContext.logDevDisp.cmdBindPipeline(
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        vulkan.pipelineContext.graphicsPipeline);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)vulkan.swapchainContext.swapchain.extent.width;
    viewport.height = (float)vulkan.swapchainContext.swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vulkan.deviceContext.logDevDisp.cmdSetViewport(command_buffer, 0, 1,
                                                   &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = vulkan.swapchainContext.swapchain.extent;
    vulkan.deviceContext.logDevDisp.cmdSetScissor(command_buffer, 0, 1,
                                                  &scissor);

    VkBuffer vertexBuffers[] = {vulkan.sceneContext.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vulkan.deviceContext.logDevDisp.cmdBindVertexBuffers(
        command_buffer, 0, 1, vertexBuffers, offsets);

    vulkan.deviceContext.logDevDisp.cmdBindIndexBuffer(
        command_buffer, vulkan.sceneContext.indexBuffer, 0,
        VK_INDEX_TYPE_UINT32);

    vulkan.deviceContext.logDevDisp.cmdBindDescriptorSets(
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        vulkan.pipelineContext.pipelineLayout, 0, 1,
        &vulkan.data.descriptor_sets.at(image_index), 0, nullptr);

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

int update_uniform_buffer(Vulkan &vulkan, uint32_t current_image) {
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
        vulkan.swapchainContext.swapchain.extent.width /
            static_cast<float>(vulkan.swapchainContext.swapchain.extent.height),
        0.1f, 10.0f);
    ubo.projection[1][1] *= -1;

    memcpy(vulkan.data.uniform_buffers_mapped.at(current_image), &ubo,
           sizeof(ubo));
    return 0;
}

}  // namespace mpvgl::vlk
