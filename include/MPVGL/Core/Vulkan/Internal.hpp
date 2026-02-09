#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Vulkan/Init.hpp"

namespace mpvgl::vlk {

VkFormat find_depth_format(Vulkan &vulkan);
GLFWwindow *create_window_glfw(const char *window_name, bool resize);
void destroy_window_glfw(GLFWwindow *window);
VkSurfaceKHR create_surface_glfw(VkInstance instance, GLFWwindow *window,
                                 VkAllocationCallbacks *allocator);
std::vector<char> readFile(const std::string &filename);
VkShaderModule createShaderModule(Vulkan &vulkan,
                                  const std::vector<char> &code);
uint32_t find_memory_type(Vulkan &vulkan, uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);
void create_buffer(Vulkan &vulkan, VkDeviceSize size, VkBufferUsageFlags usage,
                   VkMemoryPropertyFlags properties, VkBuffer &buffer,
                   VkDeviceMemory &bufferMemory);
VkCommandBuffer beginSingleTimeCommands(Vulkan &vulkan);
void endSingleTimeCommands(Vulkan &vulkan, VkCommandBuffer commandBuffer);
void copy_buffer(Vulkan &vulkan, VkBuffer srcBuffer, VkBuffer dstBuffer,
                 VkDeviceSize size);
void createImage(Vulkan &vulkan, uint32_t width, uint32_t height,
                 VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                 VkMemoryPropertyFlags properties, VkImage &image,
                 VkDeviceMemory &imageMemory);
void transition_image_layout(Vulkan &vulkan, VkImage image, VkFormat format,
                             VkImageLayout oldLayout, VkImageLayout newLayout);
void copy_buffer_to_image(Vulkan &vulkan, VkBuffer buffer, VkImage image,
                          uint32_t width, uint32_t height);
VkImageView createImageView(Vulkan &vulkan, VkImage image, VkFormat format,
                            VkImageAspectFlags aspectFlags);
int create_image_views(Vulkan &vulkan);
VkFormat find_supported_format(Vulkan &vulkan,
                               const std::vector<VkFormat> &candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features);
VkFormat find_depth_format(Vulkan &vulkan);
bool has_stencil_component(VkFormat format);
void cleanupSwapChain(Vulkan &vulkan);
int recreate_swapchain(Vulkan &vulkan);
int record_command_buffer(Vulkan &vulkan, VkCommandBuffer command_buffer,
                          uint32_t image_index);
int update_uniform_buffer(Vulkan &vulkan, uint32_t current_image);

}  // namespace mpvgl::vlk
