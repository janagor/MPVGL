#pragma once

#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

namespace mpvgl::vlk {

struct SwapchainBuilder {
    static tl::expected<vkb::Swapchain, std::error_code> getSwapchain(
        vkb::Device const& device, GLFWwindow* window,
        vkb::Swapchain& oldSwapchain) {
        vkb::SwapchainBuilder swapchainBuilder{device};
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        auto newSwapchain =
            swapchainBuilder
                .set_desired_extent(static_cast<uint32_t>(width),
                                    static_cast<uint32_t>(height))
                .set_old_swapchain(oldSwapchain)
                .build();
        if (!newSwapchain) return tl::unexpected(newSwapchain.error());
        vkb::destroy_swapchain(oldSwapchain);
        return newSwapchain.value();
    }
};

}  // namespace mpvgl::vlk
