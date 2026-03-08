#pragma once

#include <cstdint>

#include <VulkanMemoryAllocator/include/vk_mem_alloc.h>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

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

}  // namespace mpvgl::vlk
