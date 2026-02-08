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
tl::expected<void, std::error_code> device_initialization(Init &init);
tl::expected<void, std::error_code> create_swapchain(Init &init);
tl::expected<void, std::error_code> get_queues(Init &init, RenderData &data);
int create_render_pass(Init &init, RenderData &data);
std::vector<char> readFile(const std::string &filename);
VkShaderModule createShaderModule(Init &init, const std::vector<char> &code);
int create_descriptor_set_layout(Init &init, RenderData &data);
int create_graphics_pipeline(Init &init, RenderData &data);
int create_command_pool(Init &init, RenderData &data);
int create_depth_resources(Init &init, RenderData &data);
int create_framebuffers(Init &init, RenderData &data);
int create_texture_image(Init &init, RenderData &data);
int create_texture_image_view(Init &init, RenderData &data);
int create_texture_sampler(Init &init, RenderData &data);
int load_model(Init &init, RenderData &data);
int create_vertex_buffer(Init &init, RenderData &data);
int create_index_buffer(Init &init, RenderData &data);
int create_uniform_buffers(Init &init, RenderData &data);
int create_descriptor_pool(Init &init, RenderData &data);
int create_descriptor_sets(Init &init, RenderData &data);
int create_command_buffers(Init &init, RenderData &data);
int create_sync_objects(Init &init, RenderData &data);
int recreate_swapchain(Init &init, RenderData &data);
int record_command_buffer(Init &init, RenderData &data,
                          VkCommandBuffer command_buffer, uint32_t image_index);
int draw_frame(Init &init, RenderData &data);
int reloadShadersAndPipeline(Init &init, RenderData &data);
void cleanup(Init &init, RenderData &data);

}  // namespace mpvgl::vlk
