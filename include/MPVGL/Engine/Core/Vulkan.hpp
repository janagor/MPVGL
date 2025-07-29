#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <VkBootstrap.h>
#include <glm/glm.hpp>
#include <tl/expected.hpp>
#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include "MPVGL/Engine/Core/Color.hpp"

namespace mpvgl {

const int MAX_FRAMES_IN_FLIGHT = 2;

struct Vertex {
  Vertex(glm::vec2 pos, Color color)
      : pos(pos), color({color.m_red, color.m_green, color.m_blue}) {}

  glm::vec2 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 2>
  getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    return attributeDescriptions;
  }
};

struct Init {
  GLFWwindow *window;
  vkb::Instance instance;
  vkb::InstanceDispatchTable inst_disp;
  VkSurfaceKHR surface;
  vkb::Device device;
  vkb::DispatchTable disp;
  vkb::Swapchain swapchain;
};

struct RenderData {
  VkQueue graphics_queue;
  VkQueue present_queue;

  std::vector<VkImage> swapchain_images;
  std::vector<VkImageView> swapchain_image_views;
  std::vector<VkFramebuffer> framebuffers;

  VkRenderPass render_pass;
  VkPipelineLayout pipeline_layout;
  VkPipeline graphics_pipeline;

  VkCommandPool command_pool;
  std::vector<VkCommandBuffer> command_buffers;

  VkBuffer vertex_buffer;
  VkDeviceMemory vertex_buffer_memory;
  VkBuffer index_buffer;
  VkDeviceMemory index_buffer_memory;

  std::vector<VkSemaphore> available_semaphores;
  std::vector<VkSemaphore> finished_semaphore;
  std::vector<VkFence> in_flight_fences;
  std::vector<VkFence> image_in_flight;
  size_t current_frame = 0;
};

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
int create_graphics_pipeline(Init &init, RenderData &data);
int create_framebuffers(Init &init, RenderData &data);
int create_command_pool(Init &init, RenderData &data);
int create_vertex_buffer(Init &init, RenderData &data);
int create_index_buffer(Init &init, RenderData &data);
int create_command_buffers(Init &init, RenderData &data);
int create_sync_objects(Init &init, RenderData &data);
int recreate_swapchain(Init &init, RenderData &data);
int record_command_buffer(Init &init, RenderData &data,
                          VkCommandBuffer command_buffer, uint32_t image_index);
int draw_frame(Init &init, RenderData &data);
void cleanup(Init &init, RenderData &data);

} // namespace mpvgl
