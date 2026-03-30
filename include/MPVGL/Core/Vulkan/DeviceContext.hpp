#pragma once

#include <VulkanMemoryAllocator/include/vk_mem_alloc.h>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vk-bootstrap/src/VkBootstrapDispatch.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <GLFW/glfw3.h>

#include "MPVGL/Utility/Types.hpp"

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
    u32 graphicsQueueIndex{0};
    u32 presentQueueIndex{0};
};

}  // namespace mpvgl::vlk
