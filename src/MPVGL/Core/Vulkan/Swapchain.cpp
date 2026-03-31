#include <cstddef>
#include <utility>
#include <vector>

#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vk-bootstrap/src/VkBootstrapDispatch.h>
#include <vulkan/vulkan_core.h>

#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Core/Vulkan/Initializers.hpp"
#include "MPVGL/Core/Vulkan/Swapchain.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl::vlk {

Swapchain::Swapchain(vkb::Swapchain vkbSwapchain,
                     vkb::DispatchTable disp) noexcept
    : m_swapchain(vkbSwapchain), m_images({}), m_imageViews({}), m_disp(disp) {
    if (auto images = m_swapchain.get_images(); images.has_value()) {
        m_images = images.value();
    }
}

Swapchain::Swapchain(Swapchain&& other) noexcept
    : m_swapchain(other.m_swapchain),
      m_images(std::move(other.m_images)),
      m_imageViews(std::move(other.m_imageViews)),
      m_disp(other.m_disp) {
    other.m_swapchain.swapchain = VK_NULL_HANDLE;
}

auto Swapchain::operator=(Swapchain&& other) noexcept -> Swapchain& {
    if (this != &other) {
        cleanup();
        m_swapchain = other.m_swapchain;
        m_images = std::move(other.m_images);
        m_imageViews = std::move(other.m_imageViews);
        m_disp = other.m_disp;
        other.m_swapchain.swapchain = VK_NULL_HANDLE;
    }
    return *this;
}

Swapchain::~Swapchain() { cleanup(); }

void Swapchain::cleanup() noexcept {
    if (m_swapchain.swapchain != VK_NULL_HANDLE) {
        m_swapchain.destroy_image_views(m_imageViews);
        vkb::destroy_swapchain(m_swapchain);
        m_swapchain.swapchain = VK_NULL_HANDLE;
    }
    m_imageViews.clear();
    m_images.clear();
}

auto Swapchain::create(DeviceContext const& deviceContext)
    -> tl::expected<Swapchain, Error<EngineError>> {
    auto vkbSwapchain = SwapchainBuilder::getSwapchain(
        deviceContext.logicalDevice, deviceContext.window);
    if (!vkbSwapchain) {
        return tl::unexpected{vkbSwapchain.error()};
    }

    Swapchain swapchain(vkbSwapchain.value(), deviceContext.logDevDisp);
    if (auto res = swapchain.initImageViews(); !res.has_value()) {
        return tl::unexpected{res.error()};
    }
    return swapchain;
}

auto Swapchain::recreate(DeviceContext const& deviceContext)
    -> tl::expected<void, Error<EngineError>> {
    auto newSwapchain = SwapchainBuilder::getSwapchain(
        deviceContext.logicalDevice, deviceContext.window, m_swapchain);
    if (!newSwapchain) {
        return tl::unexpected{newSwapchain.error()};
    }

    cleanup();
    m_swapchain = newSwapchain.value();
    if (auto images = m_swapchain.get_images(); images.has_value()) {
        m_images = images.value();
    } else {
        m_images = std::vector<VkImage>{};
    }

    return initImageViews();
}

auto Swapchain::initImageViews() -> tl::expected<void, Error<EngineError>> {
    m_imageViews.resize(m_images.size());
    for (size_t i = 0; i < m_images.size(); ++i) {
        auto subresourceRange = VkImageSubresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };
        auto viewInfo = initializers::imageViewCreateInfo(
            m_images[i], VK_IMAGE_VIEW_TYPE_2D, m_swapchain.image_format,
            subresourceRange);

        if (m_disp.createImageView(&viewInfo, nullptr, &m_imageViews[i]) !=
            VK_SUCCESS) {
            return tl::unexpected{
                Error{EngineError::VulkanRuntimeError,
                      "Failed to create Swapchain Image View"}};
        }
    }
    return {};
}

}  // namespace mpvgl::vlk
