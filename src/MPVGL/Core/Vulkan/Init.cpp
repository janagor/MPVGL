#include "MPVGL/Core/Vulkan/Init.hpp"

namespace mpvgl::vlk {

Vulkan::RenderData::RenderData(Vulkan &vulkan) : texture(vulkan) {}

Vulkan::Vulkan() : data(*this) {}
}  // namespace mpvgl::vlk
