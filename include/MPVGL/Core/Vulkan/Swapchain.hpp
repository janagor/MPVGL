#pragma once

#include <string>
#include <vector>

#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vk-bootstrap/src/VkBootstrapDispatch.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Vulkan/DeviceContext.hpp"
#include "MPVGL/Error/EngineError.hpp"
#include "MPVGL/Error/Error.hpp"
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl::vlk {

struct SwapchainBuilder {
    static auto getSwapchain(vkb::Device const& device, GLFWwindow* window,
                             vkb::Swapchain const& oldSwapchain = {})
        -> tl::expected<vkb::Swapchain, Error<EngineError>> {
        vkb::SwapchainBuilder swapchainBuilder{device};
        int width{};
        int height{};
        glfwGetWindowSize(window, &width, &height);
        auto newSwapchain = swapchainBuilder
                                .set_desired_extent(static_cast<u32>(width),
                                                    static_cast<u32>(height))
                                .set_old_swapchain(oldSwapchain)
                                .build();
        if (!newSwapchain) {
            std::string const errorMsg =
                "Swapchain build error: " + newSwapchain.error().message();
            return tl::unexpected{
                mpvgl::Error<EngineError>{newSwapchain.error(), errorMsg}};
        }
        return newSwapchain.value();
    }
};

class Swapchain {
   public:
    Swapchain() = default;
    Swapchain(Swapchain const&) = delete;
    auto operator=(Swapchain const&) -> Swapchain& = delete;
    Swapchain(Swapchain&& other) noexcept;
    auto operator=(Swapchain&& other) noexcept -> Swapchain&;
    ~Swapchain();

    [[nodiscard]] static auto create(DeviceContext const& deviceContext)
        -> tl::expected<Swapchain, Error<EngineError>>;
    [[nodiscard]] auto recreate(DeviceContext const& deviceContext)
        -> tl::expected<void, Error<EngineError>>;

    [[nodiscard]] auto handle() const noexcept -> VkSwapchainKHR {
        return m_swapchain.swapchain;
    }
    [[nodiscard]] auto format() const noexcept -> VkFormat {
        return m_swapchain.image_format;
    }
    [[nodiscard]] auto extent() const noexcept -> VkExtent2D const& {
        return m_swapchain.extent;
    }
    [[nodiscard]] auto imageCount() const noexcept -> u32 {
        return m_swapchain.image_count;
    }
    [[nodiscard]] auto images() const noexcept -> std::vector<VkImage> const& {
        return m_images;
    }
    [[nodiscard]] auto images() noexcept -> std::vector<VkImage>& {
        return m_images;
    }
    [[nodiscard]] auto imageViews() const noexcept
        -> std::vector<VkImageView> const& {
        return m_imageViews;
    }
    [[nodiscard]] auto imageViews() noexcept -> std::vector<VkImageView>& {
        return m_imageViews;
    }

   private:
    vkb::Swapchain m_swapchain;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;
    // TODO: Make it a pointer to table owned by VulkanContext/Vulkan
    vkb::DispatchTable m_disp;

    Swapchain(vkb::Swapchain vkbSwapchain, vkb::DispatchTable disp) noexcept;
    void cleanup() noexcept;
    auto initImageViews() -> tl::expected<void, Error<EngineError>>;
};

}  // namespace mpvgl::vlk
