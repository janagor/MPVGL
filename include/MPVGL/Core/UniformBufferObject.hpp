#pragma once

#include <cstdint>

#include <glm/glm.hpp>

namespace mpvgl {

struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
};

} // namespace mpvgl
