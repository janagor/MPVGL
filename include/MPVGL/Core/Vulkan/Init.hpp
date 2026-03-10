#pragma once

#include <cstddef>
#include <vector>

#include <glm/ext/vector_float3.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Camera.hpp"
#include "MPVGL/Core/Vulkan/Buffer.hpp"
#include "MPVGL/Core/Vulkan/Descriptor.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Core/Vulkan/GraphicsPipeline.hpp"
#include "MPVGL/Core/Vulkan/Model.hpp"
#include "MPVGL/Core/Vulkan/Swapchain.hpp"
#include "MPVGL/Core/Vulkan/Texture.hpp"

namespace mpvgl::vlk {

struct SwapchainContext {
    SwapchainContext() = default;

    Swapchain swapchain{};
    Texture depthTexture{};
    VkRenderPass renderPass{VK_NULL_HANDLE};
    std::vector<VkFramebuffer> framebuffers{};
};

struct SceneContext {
    SceneContext() = default;

    Model model{};

    Texture texture{};

    Camera camera{glm::vec3{2.0f, 2.0f, 2.0f}};
};

struct PipelineContext {
    PipelineContext() = default;

    GraphicsPipeline graphicsPipeline{};
    VkDescriptorSetLayout descriptorSetLayout{};
    // VkPipelineLayout pipelineLayout{};
    // VkPipeline graphicsPipeline{};
};

class FrameData {
   public:
    FrameData() = default;

    VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
    Buffer uniformBuffer{};
    void *uniformBufferMapped{nullptr};
    VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
    VkSemaphore availableSemaphore{VK_NULL_HANDLE};
    VkFence inFlightFence{VK_NULL_HANDLE};
};

struct Vulkan {
    Vulkan() = default;

    DeviceContext deviceContext{};
    SwapchainContext swapchainContext{};
    SceneContext sceneContext{};
    PipelineContext pipelineContext{};

    VkSurfaceKHR surface{};

    struct RenderData {
        RenderData() = default;

        VkCommandPool command_pool{};
        DescriptorAllocator descriptorAllocator{};

        std::vector<FrameData> frames{};
        std::vector<VkSemaphore> finishedSemaphores{};

        std::vector<VkFence> image_in_flight{};
        size_t current_frame{};
    };

    RenderData data{};
};

}  // namespace mpvgl::vlk
