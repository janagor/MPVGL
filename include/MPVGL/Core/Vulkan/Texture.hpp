#pragma once

#include <vulkan/vulkan.h>

namespace mpvgl::vlk {

struct Vulkan;

struct Texture {
    Texture(Vulkan& vulkan);
    VkImage image;
    VkImageView imageView;
    VkSampler sampler;
    VkDeviceMemory imageMemory;

    Vulkan& vulkan;
};

}  // namespace mpvgl::vlk
