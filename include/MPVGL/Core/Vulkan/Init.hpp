#pragma once

#include <cstddef>
#include <deque>
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

class MaterialData {
   public:
    Texture texture;
    std::vector<VkDescriptorSet> descriptorSets;
};

class RenderObject {
   public:
    Model* model;
    MaterialData* material;
    glm::mat4 transformMatrix;
};

class SwapchainContext {
   public:
    Swapchain swapchain{};
    Texture depthTexture{};
    RenderPass renderPass{};
    std::vector<VkFramebuffer> framebuffers{};
};

class SceneContext {
   public:
    std::deque<Model> models{};
    std::unordered_map<std::string, MaterialData> materials{};
    std::vector<RenderObject> renderables{};
    Camera camera{glm::dvec3{2.0, 2.0, 2.0}};
};

class PipelineContext {
   public:
    GraphicsPipeline graphicsPipeline{};
    DescriptorSetLayout descriptorSetLayout{};
};

class FrameData {
   public:
    VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
    Buffer uniformBuffer{};
    void* uniformBufferMapped{nullptr};
    Semaphore availableSemaphore{};
    Fence inFlightFence{};
};

class RenderData {
   public:
    CommandPool commandPool{};
    DescriptorAllocator descriptorAllocator{};

    std::vector<FrameData> frames{};
    std::vector<Semaphore> finishedSemaphores{};

    std::vector<VkFence> imageInFlight{};
    size_t currentFrame{};
};

class Vulkan {
   public:
    DeviceContext deviceContext{};
    SwapchainContext swapchainContext{};
    SceneContext sceneContext{};
    PipelineContext pipelineContext{};

    VkSurfaceKHR surface{};
    RenderData data{};
};

}  // namespace mpvgl::vlk
