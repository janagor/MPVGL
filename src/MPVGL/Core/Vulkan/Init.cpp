#include "MPVGL/Core/Vulkan/Init.hpp"

namespace mpvgl::vlk {

SceneContext::SceneContext(Vulkan &vulkan) : texture(vulkan) {}
Vulkan::RenderData::RenderData() {}

Vulkan::Vulkan() : data(), sceneContext(*this) {}
}  // namespace mpvgl::vlk
