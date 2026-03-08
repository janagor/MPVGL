#include <vulkan/vulkan.h>

#include "MPVGL/Core/Vulkan/Init.hpp"
#include "MPVGL/Core/Vulkan/Texture.hpp"

namespace mpvgl::vlk {

Texture::Texture(Vulkan& vulkan)
    : mipLevels(0),
      image(VK_NULL_HANDLE),
      imageView(VK_NULL_HANDLE),
      sampler(VK_NULL_HANDLE),
      imageAllocation(VK_NULL_HANDLE),
      vulkan(vulkan) {}

}  // namespace mpvgl::vlk
