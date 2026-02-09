#pragma once

#include <VkBootstrap.h>

#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <tl/expected.hpp>
#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Vulkan/Init.hpp"

namespace mpvgl::vlk {

// publicly used (by RenderWindow)
tl::expected<void, std::error_code> device_initialization(Vulkan::Init &init);
tl::expected<void, std::error_code> create_swapchain(Vulkan::Init &init);
tl::expected<void, std::error_code> get_queues(Vulkan &vulkan);
int create_render_pass(Vulkan &vulkan);
int create_descriptor_set_layout(Vulkan &vulkan);
int create_graphics_pipeline(Vulkan &vulkan);
int create_command_pool(Vulkan &vulkan);
int create_depth_resources(Vulkan &vulkan);
int create_framebuffers(Vulkan &vulkan);
int create_texture_image(Vulkan &vulkan);
int create_texture_image_view(Vulkan &vulkan);
int create_texture_sampler(Vulkan &vulkan);
int load_model(Vulkan &vulkan);
int create_vertex_buffer(Vulkan &vulkan);
int create_index_buffer(Vulkan &vulkan);
int create_uniform_buffers(Vulkan &vulkan);
int create_descriptor_pool(Vulkan &vulkan);
int create_descriptor_sets(Vulkan &vulkan);
int create_command_buffers(Vulkan &vulkan);
int create_sync_objects(Vulkan &vulkan);
int draw_frame(Vulkan &vulkan);
int reloadShadersAndPipeline(Vulkan &vulkan);
void cleanup(Vulkan &vulkan);

// not used by RenderWindow
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
