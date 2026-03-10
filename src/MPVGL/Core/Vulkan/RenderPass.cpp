#include <utility>

#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Core/Vulkan/RenderPass.hpp"

namespace mpvgl::vlk {

RenderPass::RenderPass(VkRenderPass renderPass,
                       vkb::DispatchTable disp) noexcept
    : m_renderPass(renderPass), m_disp(std::move(disp)) {}

RenderPass::RenderPass(RenderPass&& other) noexcept
    : m_renderPass(other.m_renderPass), m_disp(std::move(other.m_disp)) {
    other.m_renderPass = VK_NULL_HANDLE;
}

RenderPass& RenderPass::operator=(RenderPass&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_renderPass = other.m_renderPass;
        m_disp = std::move(other.m_disp);
        other.m_renderPass = VK_NULL_HANDLE;
    }
    return *this;
}

RenderPass::~RenderPass() noexcept { cleanup(); }

void RenderPass::cleanup() noexcept {
    if (m_renderPass != VK_NULL_HANDLE) {
        m_disp.destroyRenderPass(m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }
}

RenderPassBuilder& RenderPassBuilder::addAttachment(
    VkAttachmentDescription const& attachment) {
    m_attachments.push_back(attachment);
    return *this;
}

RenderPassBuilder& RenderPassBuilder::addSubpass(SubpassInfo const& subpass) {
    m_subpassInfos.push_back(subpass);
    return *this;
}

RenderPassBuilder& RenderPassBuilder::addDependency(
    VkSubpassDependency const& dependency) {
    m_dependencies.push_back(dependency);
    return *this;
}

tl::expected<RenderPass, Error<EngineError>> RenderPassBuilder::build(
    DeviceContext const& device) {
    std::vector<VkSubpassDescription> subpasses;
    subpasses.reserve(m_subpassInfos.size());

    for (auto const& info : m_subpassInfos) {
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = info.bindPoint;
        subpass.colorAttachmentCount =
            static_cast<uint32_t>(info.colorAttachments.size());
        subpass.pColorAttachments = info.colorAttachments.data();

        if (info.depthAttachment.has_value()) {
            subpass.pDepthStencilAttachment = &info.depthAttachment.value();
        }
        subpasses.push_back(subpass);
    }

    auto renderPassInfo = initializers::renderPassCreateInfo(
        m_attachments, subpasses, m_dependencies, 0);

    VkRenderPass renderPass;
    if (device.logDevDisp.createRenderPass(&renderPassInfo, nullptr,
                                           &renderPass) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Render Pass"}};
    }

    return RenderPass{renderPass, device.logDevDisp};
}

}  // namespace mpvgl::vlk
