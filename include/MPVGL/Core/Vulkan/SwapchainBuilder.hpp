#pragma once

#include <tl/expected.hpp>
#include <vk-bootstrap/src/VkBootstrap.h>
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#include "MPVGL/Core/Error.hpp"

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

}  // namespace mpvgl::vlk
