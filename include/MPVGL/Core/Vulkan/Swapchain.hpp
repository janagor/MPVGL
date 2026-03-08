#pragma once

#include <vector>

#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Error.hpp"
#include "MPVGL/Core/Vulkan/DeviceContext.hpp"

namespace mpvgl::vlk {

struct SwapchainBuilder {
    static tl::expected<vkb::Swapchain, Error> getSwapchain(
        vkb::Device const& device, GLFWwindow* window,
        vkb::Swapchain const& oldSwapchain = {}) {
        vkb::SwapchainBuilder swapchainBuilder{device};
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        auto newSwapchain =
            swapchainBuilder
                .set_desired_extent(static_cast<uint32_t>(width),
                                    static_cast<uint32_t>(height))
                .set_old_swapchain(oldSwapchain)
                .build();
        if (!newSwapchain) {
            std::string errorMsg =
                "Swapchain build error: " + newSwapchain.error().message();
            return tl::unexpected(mpvgl::Error{newSwapchain.error(), errorMsg});
        }
        return newSwapchain.value();
    }
};

class Swapchain {
   public:
    Swapchain() = default;
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    Swapchain(Swapchain&& other) noexcept;
    Swapchain& operator=(Swapchain&& other) noexcept;
    ~Swapchain();

    [[nodiscard]] static tl::expected<Swapchain, Error> create(
        DeviceContext const& deviceContext);
    [[nodiscard]] tl::expected<void, Error> recreate(
        DeviceContext const& deviceContext);

    [[nodiscard]] VkSwapchainKHR handle() const noexcept {
        return m_swapchain.swapchain;
    }
    [[nodiscard]] VkFormat format() const noexcept {
        return m_swapchain.image_format;
    }
    [[nodiscard]] VkExtent2D const& extent() const noexcept {
        return m_swapchain.extent;
    }
    [[nodiscard]] uint32_t imageCount() const noexcept {
        return m_swapchain.image_count;
    }
    [[nodiscard]] std::vector<VkImage> const& images() const noexcept {
        return m_images;
    }
    [[nodiscard]] std::vector<VkImage>& images() noexcept { return m_images; }
    [[nodiscard]] std::vector<VkImageView> const& imageViews() const noexcept {
        return m_imageViews;
    }
    [[nodiscard]] std::vector<VkImageView>& imageViews() noexcept {
        return m_imageViews;
    }

   private:
    vkb::Swapchain m_swapchain;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;
    // TODO: Make it a pointer to table owned by VulkanContext/Vulkan
    vkb::DispatchTable m_disp;

   private:
    Swapchain(vkb::Swapchain vkbSwapchain, vkb::DispatchTable disp) noexcept;
    void cleanup() noexcept;
    tl::expected<void, Error> initImageViews();
};

}  // namespace mpvgl::vlk
