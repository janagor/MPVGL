#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include <tl/expected.hpp>
#include <vulkan/vulkan_core.h>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Vulkan/Buffer.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::vlk {

tl::expected<GLFWwindow *, Error<EngineError>> createWindow(
    int width, int height, std::string const &title, GLFWmonitor *monitor,
    GLFWwindow *share, bool resize);
void destroy_window_glfw(GLFWwindow *window);
VkSurfaceKHR create_surface_glfw(VkInstance instance, GLFWwindow *window,
                                 VkAllocationCallbacks *allocator);
VkShaderModule createShaderModule(DeviceContext const &context,
                                  std::span<std::byte const> code);
tl::expected<void, Error<EngineError>> createBuffer(
    Vulkan &vulkan, VkDeviceSize size, VkBufferUsageFlags usage,
    VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags allocFlags,
    VkBuffer &buffer, VmaAllocation &bufferAllocation);
VkCommandBuffer beginSingleTimeCommands(Vulkan &vulkan);
void endSingleTimeCommands(Vulkan &vulkan, VkCommandBuffer commandBuffer);
void copy_buffer(Vulkan &vulkan, VkBuffer srcBuffer, VkBuffer dstBuffer,
                 VkDeviceSize size);
tl::expected<void, Error<EngineError>> createImage(
    Vulkan &vulkan, Extent2D const &extent, u32 mipLevels, VkFormat format,
    VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memoryUsage,
    VmaAllocationCreateFlags allocFlags, VkImage &image,
    VmaAllocation &imageAllocation);
tl::expected<VkImageView, Error<EngineError>> createImageView(
    Vulkan &vulkan, VkImage image, VkFormat format,
    VkImageAspectFlags aspectFlags, u32 mipLevels);
tl::expected<void, Error<EngineError>> createImageViews(Vulkan &vulkan);
tl::expected<VkFormat, Error<EngineError>> findSupportedFormat(
    Vulkan &vulkan, std::vector<VkFormat> const &candidates,
    VkImageTiling tiling, VkFormatFeatureFlags features);
tl::expected<VkFormat, Error<EngineError>> findDepthFormat(Vulkan &vulkan);
bool has_stencil_component(VkFormat format);
void cleanupSwapChain(Vulkan &vulkan);
tl::expected<void, Error<EngineError>> recreateSwapchain(Vulkan &vulkan);
tl::expected<void, Error<EngineError>> recordCommandBuffer(
    Vulkan &vulkan, VkCommandBuffer command_buffer, u32 image_index);
void updateUniformBuffer(Vulkan &vulkan, u32 current_image);

}  // namespace mpvgl::vlk
