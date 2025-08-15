#pragma once

#include <array>
#include <vector>

#include <VkBootstrap.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Color.hpp"

namespace mpvgl {

struct Vertex {
  Vertex(glm::vec2 pos, Color color)
      : pos(pos), color({color.m_red, color.m_green, color.m_blue}) {}

  glm::vec2 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDescription() noexcept {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
  }

  static constexpr std::array<VkVertexInputAttributeDescription, 2>
  getAttributeDescriptions() noexcept {
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

} // namespace mpvgl
