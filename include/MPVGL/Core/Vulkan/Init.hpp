#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <array>
#include <unordered_map>
#include <vector>

#include <VulkanMemoryAllocator/include/vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Camera.hpp"
#include "MPVGL/Core/Vulkan/Texture.hpp"
#include "MPVGL/Graphics/Color.hpp"

namespace mpvgl {

struct Texture;

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
        std::array<VkVertexInputAttributeDescription, 3>
            attributeDescriptions{};

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
    bool operator==(Vertex const &other) const {
        return pos == other.pos && color == other.color &&
               texCoord == other.texCoord;
    }
};

}  // namespace mpvgl

namespace std {

template <>
struct hash<mpvgl::Vertex> {
    size_t operator()(mpvgl::Vertex const &vertex) const {
        return ((hash<glm::vec3>()(vertex.pos) ^
                 (hash<glm::vec3>()(vertex.color) << 1)) >>
                1) ^
               (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};

}  // namespace std

namespace mpvgl::vlk {

struct DeviceContext {
    GLFWwindow *window{nullptr};
    vkb::Instance instance;
    vkb::InstanceDispatchTable instDisp;

    vkb::Device logicalDevice;
    vkb::DispatchTable logDevDisp;

    VmaAllocator allocator{VK_NULL_HANDLE};

    VkQueue graphicsQueue{VK_NULL_HANDLE};
    VkQueue presentQueue{VK_NULL_HANDLE};
    std::uint32_t graphicsQueueIndex{0};
    std::uint32_t presentQueueIndex{0};
};

struct SwapchainContext {
    vkb::Swapchain swapchain;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;

    VkImage depthImage{VK_NULL_HANDLE};
    VkDeviceMemory depthImageMemory{VK_NULL_HANDLE};
    VkImageView depthImageView{VK_NULL_HANDLE};

    VkRenderPass renderPass{VK_NULL_HANDLE};
    std::vector<VkFramebuffer> framebuffers;
};

struct SceneContext {
    SceneContext(Vulkan &vulkan);

    std::vector<Vertex> vertices;
    std::unordered_map<Vertex, uint32_t> uniqueVertices;
    VkBuffer vertexBuffer;
    VmaAllocation vertexBufferAllocation;

    std::vector<uint32_t> indices;
    VkBuffer indexBuffer;
    VmaAllocation indexBufferAllocation;

    Texture texture;

    Camera camera{glm::vec3(2.0f, 2.0f, 2.0f)};
};

struct PipelineContext {
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
};

struct Vulkan {
    Vulkan();
    DeviceContext deviceContext;
    SwapchainContext swapchainContext;
    SceneContext sceneContext;
    PipelineContext pipelineContext;

    VkSurfaceKHR surface;

    struct RenderData {
        RenderData();

        VkCommandPool command_pool;
        std::vector<VkCommandBuffer> command_buffers;

        std::vector<VkBuffer> uniform_buffers;
        std::vector<VmaAllocation> uniformBufferAllocations;
        std::vector<void *> uniform_buffers_mapped;

        VkDescriptorPool descriptor_pool;
        std::vector<VkDescriptorSet> descriptor_sets;

        std::vector<VkSemaphore> available_semaphores;
        std::vector<VkSemaphore> finished_semaphore;
        std::vector<VkFence> in_flight_fences;
        std::vector<VkFence> image_in_flight;
        size_t current_frame = 0;
    };

    RenderData data;
};

}  // namespace mpvgl::vlk
