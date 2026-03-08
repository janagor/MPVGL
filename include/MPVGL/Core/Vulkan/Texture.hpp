#pragma once

#include <vulkan/vulkan.h>

namespace mpvgl::vlk {

struct Vulkan;

struct Texture {
    Texture(Vulkan& vulkan);
    uint32_t mipLevels;
    VkImage image;
    VkImageView imageView;
    VkSampler sampler;
    VmaAllocation imageAllocation;

    Vulkan& vulkan;
};

}  // namespace mpvgl::vlk
