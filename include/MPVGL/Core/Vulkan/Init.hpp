#pragma once

#include <cstddef>
#include <vector>

#include <glm/ext/vector_float3.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Camera.hpp"
#include "MPVGL/Core/Vulkan/Buffer.hpp"
#include "MPVGL/Core/Vulkan/CommandPool.hpp"
#include "MPVGL/Core/Vulkan/Descriptor.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Core/Vulkan/GraphicsPipeline.hpp"
#include "MPVGL/Core/Vulkan/Model.hpp"
#include "MPVGL/Core/Vulkan/RenderPass.hpp"
#include "MPVGL/Core/Vulkan/Swapchain.hpp"
#include "MPVGL/Core/Vulkan/SyncObjects.hpp"
#include "MPVGL/Core/Vulkan/Texture.hpp"

namespace mpvgl::vlk {

class SwapchainContext {
   public:
    SwapchainContext() = default;

    Swapchain swapchain{};
    Texture depthTexture{};
    RenderPass renderPass{};
    std::vector<VkFramebuffer> framebuffers{};
};

class SceneContext {
   public:
    SceneContext() = default;

    Model model{};
    Texture texture{};
    Camera camera{glm::vec3{2.0f, 2.0f, 2.0f}};
};

class PipelineContext {
   public:
    PipelineContext() = default;

    GraphicsPipeline graphicsPipeline{};
    VkDescriptorSetLayout descriptorSetLayout{};
};

class FrameData {
   public:
    FrameData() = default;

    VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
    Buffer uniformBuffer{};
    void *uniformBufferMapped{nullptr};
    VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
    Semaphore availableSemaphore{};
    Fence inFlightFence{};
};

class RenderData {
   public:
    RenderData() = default;

    CommandPool commandPool{};
    DescriptorAllocator descriptorAllocator{};

    std::vector<FrameData> frames{};
    std::vector<Semaphore> finishedSemaphores{};

    std::vector<VkFence> imageInFlight{};
    size_t currentFrame{};
};

class Vulkan {
   public:
    Vulkan() = default;

    DeviceContext deviceContext{};
    SwapchainContext swapchainContext{};
    SceneContext sceneContext{};
    PipelineContext pipelineContext{};

    VkSurfaceKHR surface{};
    RenderData data{};
};

}  // namespace mpvgl::vlk
