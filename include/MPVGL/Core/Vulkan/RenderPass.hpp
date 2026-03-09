#pragma once

#include <optional>
#include <vector>

#include <tl/expected.hpp>
#include <vulkan/vulkan.h>

#include "MPVGL/Core/Error.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"

namespace mpvgl::vlk {

struct SubpassInfo {
    VkPipelineBindPoint bindPoint{VK_PIPELINE_BIND_POINT_GRAPHICS};
    std::vector<VkAttachmentReference> colorAttachments{};
    std::optional<VkAttachmentReference> depthAttachment{};
};

class RenderPassBuilder {
   public:
    RenderPassBuilder() = default;

    RenderPassBuilder& addAttachment(VkAttachmentDescription const& attachment);
    RenderPassBuilder& addSubpass(SubpassInfo const& subpass);
    RenderPassBuilder& addDependency(VkSubpassDependency const& dependency);

    tl::expected<VkRenderPass, Error> build(DeviceContext const& device);

   private:
    std::vector<VkAttachmentDescription> m_attachments{};
    std::vector<SubpassInfo> m_subpassInfos{};
    std::vector<VkSubpassDependency> m_dependencies{};
};

}  // namespace mpvgl::vlk
