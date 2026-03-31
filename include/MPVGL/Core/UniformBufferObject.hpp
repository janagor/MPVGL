#pragma once

#include <cstddef>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/glm.hpp>

namespace mpvgl::vlk {

constexpr std::size_t VULKAN_ALIGNMENT = 16;

struct UniformBufferObject {
    alignas(VULKAN_ALIGNMENT) glm::mat4 view;
    alignas(VULKAN_ALIGNMENT) glm::mat4 projection;
};

}  // namespace mpvgl::vlk
