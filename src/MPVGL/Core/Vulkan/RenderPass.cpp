#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Core/Vulkan/RenderPass.hpp"

namespace mpvgl::vlk {

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

tl::expected<VkRenderPass, Error> RenderPassBuilder::build(
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
        return tl::unexpected(Error(EngineError::VulkanRuntimeError,
                                    "Failed to create Render Pass"));
    }

    return renderPass;
}

}  // namespace mpvgl::vlk
