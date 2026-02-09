#include <iostream>

#include <stb/stb_image.h>
#include <vulkan/vulkan.h>

#include "MPVGL/Core/Vulkan.hpp"
#include "MPVGL/Core/Vulkan/Init.hpp"
#include "MPVGL/Core/Vulkan/Texture.hpp"

#include "config.hpp"

namespace mpvgl::vlk {

Texture::Texture(Vulkan& vulkan)
    : image(VK_NULL_HANDLE),
      imageView(VK_NULL_HANDLE),
      sampler(VK_NULL_HANDLE),
      imageMemory(VK_NULL_HANDLE),
      vulkan(vulkan) {}

}  // namespace mpvgl::vlk
