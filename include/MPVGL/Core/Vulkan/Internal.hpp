#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Error.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"

#include "tl/expected.hpp"

namespace mpvgl::vlk {

tl::expected<GLFWwindow *, Error> createWindow(const char *window_name,
                                               bool resize);
void destroy_window_glfw(GLFWwindow *window);
VkSurfaceKHR create_surface_glfw(VkInstance instance, GLFWwindow *window,
                                 VkAllocationCallbacks *allocator);
tl::expected<std::vector<char>, Error> readFile(const std::string &filename);
VkShaderModule createShaderModule(Vulkan &vulkan,
                                  const std::vector<char> &code);
tl::expected<uint32_t, Error> findMemoryType(Vulkan &vulkan,
                                             uint32_t typeFilter,
                                             VkMemoryPropertyFlags properties);
tl::expected<void, Error> createBuffer2(Vulkan &vulkan, VkDeviceSize size,
                                        VkBufferUsageFlags usage,
                                        VmaMemoryUsage memoryUsage,
                                        VmaAllocationCreateFlags allocFlags,
                                        VkBuffer &buffer,
                                        VmaAllocation &bufferAllocation);
tl::expected<void, Error> createBuffer(Vulkan &vulkan, VkDeviceSize size,
                                       VkBufferUsageFlags usage,
                                       VkMemoryPropertyFlags properties,
                                       VkBuffer &buffer,
                                       VkDeviceMemory &bufferMemory);
VkCommandBuffer beginSingleTimeCommands(Vulkan &vulkan);
void endSingleTimeCommands(Vulkan &vulkan, VkCommandBuffer commandBuffer);
void copy_buffer(Vulkan &vulkan, VkBuffer srcBuffer, VkBuffer dstBuffer,
                 VkDeviceSize size);
tl::expected<void, Error> createImage(Vulkan &vulkan, uint32_t width,
                                      uint32_t height, uint32_t mipLevels,
                                      VkFormat format, VkImageTiling tiling,
                                      VkImageUsageFlags usage,
                                      VkMemoryPropertyFlags properties,
                                      VkImage &image,
                                      VkDeviceMemory &imageMemory);
tl::expected<void, Error> transitionImageLayout(Vulkan &vulkan, VkImage image,
                                                VkFormat format,
                                                VkImageLayout oldLayout,
                                                VkImageLayout newLayout,
                                                uint32_t mipLevels);
tl::expected<void, Error> generateMipmaps(Vulkan &vulkan, VkImage image,
                                          VkFormat imageFormat,
                                          int32_t texWidth, int32_t texHeight,
                                          uint32_t mipLevels);
void copy_buffer_to_image(Vulkan &vulkan, VkBuffer buffer, VkImage image,
                          uint32_t width, uint32_t height);
tl::expected<VkImageView, Error> createImageView(Vulkan &vulkan, VkImage image,
                                                 VkFormat format,
                                                 VkImageAspectFlags aspectFlags,
                                                 uint32_t mipLevels);
tl::expected<void, Error> createImageViews(Vulkan &vulkan);
tl::expected<VkFormat, Error> findSupportedFormat(
    Vulkan &vulkan, const std::vector<VkFormat> &candidates,
    VkImageTiling tiling, VkFormatFeatureFlags features);
tl::expected<VkFormat, Error> findDepthFormat(Vulkan &vulkan);
bool has_stencil_component(VkFormat format);
void cleanupSwapChain(Vulkan &vulkan);
tl::expected<void, Error> recreateSwapchain(Vulkan &vulkan);
tl::expected<void, Error> recordCommandBuffer(Vulkan &vulkan,
                                              VkCommandBuffer command_buffer,
                                              uint32_t image_index);
void updateUniformBuffer(Vulkan &vulkan, uint32_t current_image);

}  // namespace mpvgl::vlk
