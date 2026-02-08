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

GLFWwindow *create_window_glfw(const char *window_name = "",
                               bool resize = true);
void destroy_window_glfw(GLFWwindow *window);
VkSurfaceKHR create_surface_glfw(VkInstance instance, GLFWwindow *window,
                                 VkAllocationCallbacks *allocator = nullptr);
tl::expected<void, std::error_code> device_initialization(Vulkan::Init& init);
tl::expected<void, std::error_code> create_swapchain(Vulkan::Init& init);
tl::expected<void, std::error_code> get_queues(Vulkan& vulkan);
int create_render_pass(Vulkan& vulkan);
std::vector<char> readFile(const std::string &filename);
VkShaderModule createShaderModule(Vulkan&vulkan, const std::vector<char> &code);
int create_descriptor_set_layout(Vulkan& vulkan);
int create_graphics_pipeline(Vulkan& vulkan);
int create_command_pool(Vulkan& vulkan);
int create_depth_resources(Vulkan& vulkan);
int create_framebuffers(Vulkan& vulkan);
int create_texture_image(Vulkan& vulkan);
int create_texture_image_view(Vulkan& vulkan);
int create_texture_sampler(Vulkan& vulkan);
int load_model(Vulkan& vulkan);
int create_vertex_buffer(Vulkan& vulkan);
int create_index_buffer(Vulkan& vulkan);
int create_uniform_buffers(Vulkan& vulkan);
int create_descriptor_pool(Vulkan& vulkan);
int create_descriptor_sets(Vulkan& vulkan);
int create_command_buffers(Vulkan& vulkan);
int create_sync_objects(Vulkan& vulkan);
int recreate_swapchain(Vulkan& vulkan);
int record_command_buffer(Vulkan&vulkan,
                          VkCommandBuffer command_buffer, uint32_t image_index);
int draw_frame(Vulkan& vulkan);
int reloadShadersAndPipeline(Vulkan& vulkan);
void cleanup(Vulkan& vulkan);

}  // namespace mpvgl::vlk
