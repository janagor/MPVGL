#pragma once

#include <cstdint>

#include <glm/glm.hpp>

namespace mpvgl {

struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 projection;
};

}  // namespace mpvgl
