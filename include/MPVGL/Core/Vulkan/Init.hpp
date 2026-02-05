#pragma once

#include <GLFW/glfw3.h>
#include <VkBootstrap.h>

#include <array>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "MPVGL/Graphics/Color.hpp"

namespace mpvgl {

struct Vertex {
  Vertex(glm::vec3 pos, Color color, glm::vec2 texCoord)
      : pos(pos),
        color({color.m_red, color.m_green, color.m_blue}),
        texCoord(texCoord) {}

  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 texCoord;

  static VkVertexInputBindingDescription getBindingDescription() noexcept {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
  }

  static constexpr std::array<VkVertexInputAttributeDescription, 3>
  getAttributeDescriptions() noexcept {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

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

  VkDescriptorSetLayout descriptor_set_layout;
  VkPipelineLayout pipeline_layout;
  VkPipeline graphics_pipeline;

  VkCommandPool command_pool;
  std::vector<VkCommandBuffer> command_buffers;

  VkBuffer vertex_buffer;
  VkDeviceMemory vertex_buffer_memory;
  VkBuffer index_buffer;
  VkDeviceMemory index_buffer_memory;

  VkImage depth_image;
  VkDeviceMemory depth_image_memory;
  VkImageView depth_image_view;

  VkImage texture_image;
  VkImageView texture_image_view;
  VkSampler texture_sampler;
  VkDeviceMemory texture_image_memory;

  std::vector<VkBuffer> uniform_buffers;
  std::vector<VkDeviceMemory> uniform_buffers_memory;
  std::vector<void *> uniform_buffers_mapped;

  VkDescriptorPool descriptor_pool;
  std::vector<VkDescriptorSet> descriptor_sets;

  std::vector<VkSemaphore> available_semaphores;
  std::vector<VkSemaphore> finished_semaphore;
  std::vector<VkFence> in_flight_fences;
  std::vector<VkFence> image_in_flight;
  size_t current_frame = 0;
};

}  // namespace mpvgl
