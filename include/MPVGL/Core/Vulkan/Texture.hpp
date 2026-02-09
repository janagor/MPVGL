#pragma once

#include <vulkan/vulkan.h>

namespace mpvgl::vlk {

struct Texture {
    VkImage image;
    VkImageView imageView;
    VkSampler sampler;
    VkDeviceMemory imageMemory;
};

}  // namespace mpvgl::vlk
