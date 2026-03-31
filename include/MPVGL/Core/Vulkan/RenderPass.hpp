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
    auto operator=(RenderPass const&) -> RenderPass& = delete;

    RenderPass(RenderPass&& other) noexcept;
    auto operator=(RenderPass&& other) noexcept -> RenderPass&;

    [[nodiscard]] auto handle() const noexcept -> VkRenderPass {
        return m_renderPass;
    }

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

    auto addAttachment(VkAttachmentDescription const& attachment)
        -> RenderPassBuilder&;
    auto addSubpass(SubpassInfo const& subpass) -> RenderPassBuilder&;
    auto addDependency(VkSubpassDependency const& dependency)
        -> RenderPassBuilder&;

    auto build(DeviceContext const& device)
        -> tl::expected<RenderPass, Error<EngineError>>;

   private:
    std::vector<VkAttachmentDescription> m_attachments;
    std::vector<SubpassInfo> m_subpassInfos;
    std::vector<VkSubpassDependency> m_dependencies;
};

}  // namespace mpvgl::vlk
