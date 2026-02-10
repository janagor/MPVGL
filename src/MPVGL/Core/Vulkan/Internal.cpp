#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include <glm/glm.hpp>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/UniformBufferObject.hpp"
#include "MPVGL/Core/Vulkan.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Core/Vulkan/Internal.hpp"

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
    if (vulkan.devDisp.createShaderModule(&create_info, nullptr,
                                          &shaderModule) != VK_SUCCESS) {
        return VK_NULL_HANDLE;  // failed to create shader module
    }

    return shaderModule;
}

uint32_t find_memory_type(Vulkan &vulkan, uint32_t typeFilter,
                          VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vulkan.device.physical_device,
                                        &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) ==
                properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void create_buffer(Vulkan &vulkan, VkDeviceSize size, VkBufferUsageFlags usage,
                   VkMemoryPropertyFlags properties, VkBuffer &buffer,
                   VkDeviceMemory &bufferMemory) {
    auto bufferInfo = initializers::bufferCreateInfo(size, usage);
    if (vkCreateBuffer(vulkan.device, &bufferInfo, nullptr, &buffer) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vulkan.device, buffer, &memRequirements);

    auto allocInfo = initializers::memoryAllocateInfo(
        memRequirements.size,
        find_memory_type(vulkan, memRequirements.memoryTypeBits, properties));

    if (vkAllocateMemory(vulkan.device, &allocInfo, nullptr, &bufferMemory) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(vulkan.device, buffer, bufferMemory, 0);
}

VkCommandBuffer beginSingleTimeCommands(Vulkan &vulkan) {
    auto allocInfo = initializers::commandBufferAllocateInfo(
        vulkan.data.command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(vulkan.device, &allocInfo, &commandBuffer);

    auto beginInfo = initializers::commandBufferBeginInfo(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(Vulkan &vulkan, VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    auto submitInfo = initializers::submitInfo({}, {}, {&commandBuffer, 1}, {});
    vkQueueSubmit(vulkan.data.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vulkan.data.graphics_queue);

    vkFreeCommandBuffers(vulkan.device, vulkan.data.command_pool, 1,
                         &commandBuffer);
}

void copy_buffer(Vulkan &vulkan, VkBuffer srcBuffer, VkBuffer dstBuffer,
                 VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(vulkan);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(vulkan, commandBuffer);
}

void createImage(Vulkan &vulkan, uint32_t width, uint32_t height,
                 uint32_t mipLevels, VkFormat format, VkImageTiling tiling,
                 VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                 VkImage &image, VkDeviceMemory &imageMemory) {
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

    if (vkCreateImage(vulkan.device, &imageInfo, nullptr, &image) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vulkan.device, image, &memRequirements);

    auto allocInfo = initializers::memoryAllocateInfo(
        memRequirements.size,
        find_memory_type(vulkan, memRequirements.memoryTypeBits, properties));

    if (vkAllocateMemory(vulkan.device, &allocInfo, nullptr, &imageMemory) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }
    vkBindImageMemory(vulkan.device, image, imageMemory, 0);
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

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(vulkan, commandBuffer);
}
void generateMipmaps(Vulkan &vulkan, VkImage image, VkFormat imageFormat,
                     int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(vulkan.device.physical_device,
                                        imageFormat, &formatProperties);
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
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &barrier);
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
        vkCmdBlitImage(
            commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &barrier);
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &barrier);

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
    vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

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
    if (vkCreateImageView(vulkan.device, &viewInfo, nullptr, &imageView) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    return imageView;
}

int create_image_views(Vulkan &vulkan) {
    vulkan.data.swapchain_image_views.resize(
        vulkan.data.swapchain_images.size());
    for (uint32_t i = 0; i < vulkan.data.swapchain_images.size(); ++i) {
        vulkan.data.swapchain_image_views.at(i) = createImageView(
            vulkan, vulkan.data.swapchain_images.at(i),
            vulkan.swapchain.image_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
    return 0;
}

VkFormat find_supported_format(Vulkan &vulkan,
                               const std::vector<VkFormat> &candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(vulkan.device.physical_device,
                                            format, &props);
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
    if (vulkan.data.depth_image_view != VK_NULL_HANDLE) {
        vulkan.devDisp.destroyImageView(vulkan.data.depth_image_view, nullptr);
        vulkan.devDisp.destroyImage(vulkan.data.depth_image, nullptr);
        vulkan.devDisp.freeMemory(vulkan.data.depth_image_memory, nullptr);
    }

    for (auto framebuffer : vulkan.data.framebuffers) {
        vulkan.devDisp.destroyFramebuffer(framebuffer, nullptr);
    }

    vulkan.swapchain.destroy_image_views(vulkan.data.swapchain_image_views);

    vkb::destroy_swapchain(vulkan.swapchain);
}

int recreate_swapchain(Vulkan &vulkan) {
    vulkan.devDisp.deviceWaitIdle();

    if (vulkan.data.depth_image_view != VK_NULL_HANDLE) {
        vulkan.devDisp.destroyImageView(vulkan.data.depth_image_view, nullptr);
        vulkan.devDisp.destroyImage(vulkan.data.depth_image, nullptr);
        vulkan.devDisp.freeMemory(vulkan.data.depth_image_memory, nullptr);
    }

    vulkan.devDisp.destroyCommandPool(vulkan.data.command_pool, nullptr);
    for (auto framebuffer : vulkan.data.framebuffers) {
        vulkan.devDisp.destroyFramebuffer(framebuffer, nullptr);
    }

    vulkan.swapchain.destroy_image_views(vulkan.data.swapchain_image_views);
    vulkan.data.framebuffers.clear();
    vulkan.data.swapchain_image_views.clear();

    if (!create_swapchain(vulkan).has_value()) return -1;
    vulkan.data.swapchain_images = vulkan.swapchain.get_images().value();
    if (0 != create_depth_resources(vulkan)) return -1;
    if (0 != create_image_views(vulkan)) return -1;
    if (0 != create_framebuffers(vulkan)) return -1;
    if (0 != create_command_pool(vulkan)) return -1;
    if (0 != create_command_buffers(vulkan)) return -1;

    return 0;
}

int record_command_buffer(Vulkan &vulkan, VkCommandBuffer command_buffer,
                          uint32_t image_index) {
    auto beginInfo = initializers::commandBufferBeginInfo();
    if (vulkan.devDisp.beginCommandBuffer(command_buffer, &beginInfo) !=
        VK_SUCCESS) {
        return -1;  // failed to begin recording command buffer
    }

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    auto renderPassInfo = initializers::renderPassBeginInfo(
        vulkan.data.render_pass, vulkan.data.framebuffers.at(image_index),
        VkRect2D{VkOffset2D{0, 0}, vulkan.swapchain.extent}, clearValues);

    vulkan.devDisp.cmdBeginRenderPass(command_buffer, &renderPassInfo,
                                      VK_SUBPASS_CONTENTS_INLINE);
    vulkan.devDisp.cmdBindPipeline(command_buffer,
                                   VK_PIPELINE_BIND_POINT_GRAPHICS,
                                   vulkan.data.graphics_pipeline);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)vulkan.swapchain.extent.width;
    viewport.height = (float)vulkan.swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vulkan.devDisp.cmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = vulkan.swapchain.extent;
    vulkan.devDisp.cmdSetScissor(command_buffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = {vulkan.data.vertex_buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(command_buffer, vulkan.data.index_buffer, 0,
                         VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            vulkan.data.pipeline_layout, 0, 1,
                            &vulkan.data.descriptor_sets.at(image_index), 0,
                            nullptr);

    vkCmdDrawIndexed(command_buffer,
                     static_cast<uint32_t>(vulkan.data.indices.size()), 1, 0, 0,
                     0);

    vulkan.devDisp.cmdEndRenderPass(command_buffer);

    if (vulkan.devDisp.endCommandBuffer(command_buffer) != VK_SUCCESS) {
        std::cout << "failed to record command buffer\n";
        return -1;  // failed to record command buffer!
    }
    return 0;
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
    ubo.view =
        glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.projection = glm::perspective(
        glm::radians(45.0f),
        vulkan.swapchain.extent.width / (float)vulkan.swapchain.extent.height,
        0.1f, 10.0f);
    ubo.projection[1][1] *= -1;

    memcpy(vulkan.data.uniform_buffers_mapped.at(current_image), &ubo,
           sizeof(ubo));
    return 0;
}

}  // namespace mpvgl::vlk
