#include <vector>

#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrapDispatch.h>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Core/Vulkan/RenderPass.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl::vlk {

RenderPass::RenderPass(VkRenderPass renderPass,
                       vkb::DispatchTable disp) noexcept
    : m_renderPass(renderPass), m_disp(disp) {}

RenderPass::RenderPass(RenderPass&& other) noexcept
    : m_renderPass(other.m_renderPass), m_disp(other.m_disp) {
    other.m_renderPass = VK_NULL_HANDLE;
}

auto RenderPass::operator=(RenderPass&& other) noexcept -> RenderPass& {
    if (this != &other) {
        cleanup();
        m_renderPass = other.m_renderPass;
        m_disp = other.m_disp;
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

auto RenderPassBuilder::addAttachment(VkAttachmentDescription const& attachment)
    -> RenderPassBuilder& {
    m_attachments.push_back(attachment);
    return *this;
}

auto RenderPassBuilder::addSubpass(SubpassInfo const& subpass)
    -> RenderPassBuilder& {
    m_subpassInfos.push_back(subpass);
    return *this;
}

auto RenderPassBuilder::addDependency(VkSubpassDependency const& dependency)
    -> RenderPassBuilder& {
    m_dependencies.push_back(dependency);
    return *this;
}

auto RenderPassBuilder::build(DeviceContext const& device)
    -> tl::expected<RenderPass, Error<EngineError>> {
    std::vector<VkSubpassDescription> subpasses;
    subpasses.reserve(m_subpassInfos.size());

    for (auto const& info : m_subpassInfos) {
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = info.bindPoint;
        subpass.colorAttachmentCount =
            static_cast<u32>(info.colorAttachments.size());
        subpass.pColorAttachments = info.colorAttachments.data();

        if (info.depthAttachment.has_value()) {
            subpass.pDepthStencilAttachment = &info.depthAttachment.value();
        }
        subpasses.push_back(subpass);
    }

    auto renderPassInfo = initializers::renderPassCreateInfo(
        m_attachments, subpasses, m_dependencies, 0);

    VkRenderPass renderPass = nullptr;
    if (device.logDevDisp.createRenderPass(&renderPassInfo, nullptr,
                                           &renderPass) != VK_SUCCESS) {
        return tl::unexpected{Error{EngineError::VulkanRuntimeError,
                                    "Failed to create Render Pass"}};
    }

    return RenderPass{renderPass, device.logDevDisp};
}

}  // namespace mpvgl::vlk
