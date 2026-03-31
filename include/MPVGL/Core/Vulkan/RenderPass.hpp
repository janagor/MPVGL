#pragma once

#include <optional>
#include <vector>

#include <tl/expected.hpp>
#include <vulkan/vulkan.h>

#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::vlk {

class RenderPass {
   public:
    RenderPass() = default;
    ~RenderPass() noexcept;

    RenderPass(RenderPass const&) = delete;
    RenderPass& operator=(RenderPass const&) = delete;

    RenderPass(RenderPass&& other) noexcept;
    RenderPass& operator=(RenderPass&& other) noexcept;

    [[nodiscard]] VkRenderPass handle() const noexcept { return m_renderPass; }

    RenderPass(VkRenderPass renderPass, vkb::DispatchTable disp) noexcept;

   private:
    void cleanup() noexcept;

    VkRenderPass m_renderPass{VK_NULL_HANDLE};
    vkb::DispatchTable m_disp;
};

struct SubpassInfo {
    VkPipelineBindPoint bindPoint{VK_PIPELINE_BIND_POINT_GRAPHICS};
    std::vector<VkAttachmentReference> colorAttachments;
    std::optional<VkAttachmentReference> depthAttachment;
};

class RenderPassBuilder {
   public:
    RenderPassBuilder() = default;

    RenderPassBuilder& addAttachment(VkAttachmentDescription const& attachment);
    RenderPassBuilder& addSubpass(SubpassInfo const& subpass);
    RenderPassBuilder& addDependency(VkSubpassDependency const& dependency);

    tl::expected<RenderPass, Error<EngineError>> build(
        DeviceContext const& device);

   private:
    std::vector<VkAttachmentDescription> m_attachments;
    std::vector<SubpassInfo> m_subpassInfos;
    std::vector<VkSubpassDependency> m_dependencies;
};

}  // namespace mpvgl::vlk
